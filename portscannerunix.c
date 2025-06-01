#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>

typedef enum {
    PROTO_TCP,
    PROTO_UDP
} Protocol;

typedef struct {
    struct sockaddr_in sa;
    uint16_t port;
    Protocol protocol;
} ScanParams;

#define MAX_THREADS 200

void *scan_port(void *arg) {
    ScanParams *params = (ScanParams *)arg;
    int sock;

    if (params->protocol == PROTO_TCP) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
    } else {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
    }
    if (sock < 0) {
        free(params);
        pthread_exit(NULL);
    }

    params->sa.sin_port = htons(params->port);

    // Set a timeout for connect to avoid long delays or false positives
    struct timeval timeout;
    timeout.tv_sec = 1;  // 1 second timeout
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    char *protocol;
    if (connect(sock, (struct sockaddr *)&params->sa, sizeof(params->sa)) == 0) {
        protocol = "tcp";
        printf("%-6d %s      open\n", params->port, protocol);
    }

    close(sock);
    free(params);
    pthread_exit(NULL);
}

void display_help() {
    printf("Usage: sscan [options]\n");
    printf("Options:\n");
    printf("  --help                    Show this help message\n");
    printf("  --version, -v             Show version\n");
    printf("  -h <addr>                 Hostname or IP address\n");
    printf("  -p <start-end>|<port>     Port range to scan\n");
}

int main(int argc, char **argv) {
    struct hostent *host = NULL;
    struct sockaddr_in sa;
    unsigned int start_port = 1;
    unsigned int end_port = 65535;
    char hostname[100] = {0};

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0) {
            display_help();
            exit(0);
        } else if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0) {
            printf("SockScan v0.1\n");
            exit(0);
        } else if (strcmp(argv[i], "-h") == 0 && i + 1 < argc) {
            strncpy(hostname, argv[++i], sizeof(hostname) - 1);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            ++i;

            if (sscanf(argv[i], "%u", &start_port) == 1 && strchr(argv[i], '-') == NULL) {
                end_port = start_port;
            } else if (sscanf(argv[i], "%u-%u", &start_port, &end_port) == 2) {
                // valid range
            } else {
                fprintf(stderr, "Invalid port specification: %s\n", argv[i]);
                exit(1);
            }
        } else {
            fprintf(stderr, "Invalid option: %s\n", argv[i]);
            display_help();
            exit(1);
        }
    }

    if (hostname[0] == '\0' || start_port == 0 || end_port == 0) {
        fprintf(stderr, "Missing required host (-h) or ports (-p) argument\n");
        exit(1);
    }

    if (start_port < 1 || end_port > 65535 || start_port > end_port) {
        fprintf(stderr, "Invalid port range. Ports are within range 1-65535\n");
        exit(1);
    }

    memset(&sa, 0, sizeof(sa));
    sa.sin_family = AF_INET;

    sa.sin_addr.s_addr = inet_addr(hostname);
    if (sa.sin_addr.s_addr == INADDR_NONE) {
        host = gethostbyname(hostname);
        if (!host) {
            herror("gethostbyname");
            exit(2);
        }
        memcpy(&sa.sin_addr, host->h_addr_list[0], host->h_length);
    }

    pthread_t threads[MAX_THREADS];
    int thread_count = 0;

    printf("%-6s %-6s %-6s %-6s %-6s\n", "PORT", "PROTOCOL", "STATE", "SERVICE", "VERSION");
    for (uint16_t port = start_port; port <= end_port; port++) {
        ScanParams *params = malloc(sizeof(ScanParams));
        if (!params) continue;

        memcpy(&params->sa, &sa, sizeof(sa));
        params->port = port;
        params->protocol = PROTO_TCP;

        pthread_create(&threads[thread_count++], NULL, scan_port, params);

        if (thread_count >= MAX_THREADS) {
            for (int j = 0; j < thread_count; j++) {
                pthread_join(threads[j], NULL);
            }
            thread_count = 0;
        }

        if (port == end_port) {
            break;
        }
    }

    for (int j = 0; j < thread_count; j++) {
        pthread_join(threads[j], NULL);
    }

    return 0;
}
