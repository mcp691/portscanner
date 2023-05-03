#include <stdio.h>
#include <winsock2.h>

#pragma comment(lib, "ws2_32.lib")

int main(int argc, char **argv) {
    WSADATA wsaData;
    SOCKET s;
    struct sockaddr_in target;
    int i, start_port, end_port;

    // Initialize Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }

    // Get target IP address
    char *ip_address;
    printf("Enter IP Address: ");
    scanf("%d" , &ip_address);
    target.sin_addr.s_addr = inet_addr(ip_address);

    // Get port range to scan
    start_port = 1;
    end_port = 65535;

    // Loop through the port range and try to connect to each port
    for (i = start_port; i <= end_port; i++) {
        // Create a socket
        s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) {
            printf("Error creating socket: %d\n", WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Set the target port number
        target.sin_family = AF_INET;
        target.sin_port = htons(i);

        // Connect to the target port
        if (connect(s, (struct sockaddr *)&target, sizeof(target)) != SOCKET_ERROR) {
            printf("Port %d is open\n", i);
            closesocket(s);
        }
    }

    // Clean up Winsock
    WSACleanup();
    return 0;
}
