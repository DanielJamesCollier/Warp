// Client Code

#include <winsock2.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define MESSAGE "Hello, Server!"
#define HEARTBEAT_MESSAGE "HEARTBEAT"
#define HEARTBEAT_INTERVAL 5000 // in milliseconds

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server;

    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Winsock initialized.\n");

    // Create socket
    if ((client_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket created.\n");

    // Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    inet_pton(AF_INET, SERVER_IP, &server.sin_addr);
    server.sin_port = htons(PORT);

    // Connect to server
    if (connect(client_socket, (struct sockaddr *)&server, sizeof(server)) < 0) {
        printf("Connect failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Connected to server.\n");

    // Send a message
    if (send(client_socket, MESSAGE, strlen(MESSAGE), 0) < 0) {
        printf("Send failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Message sent: %s\n", MESSAGE);

    // Send heartbeat messages periodically
    while (1) {
        Sleep(HEARTBEAT_INTERVAL);
        if (send(client_socket, HEARTBEAT_MESSAGE, strlen(HEARTBEAT_MESSAGE), 0) < 0) {
            printf("Heartbeat send failed. Error Code: %d\n", WSAGetLastError());
            break;
        }
        printf("Heartbeat sent.\n");
    }

    // Close socket
    closesocket(client_socket);

    WSACleanup();

    return 0;
}
