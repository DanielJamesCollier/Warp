#include <winsock2.h>
#include <stdio.h>
#include <stdbool.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024
#define HEARTBEAT_MESSAGE "HEARTBEAT"

SOCKET waitForClient(SOCKET server_socket, struct sockaddr_in *client) {
    int client_len = sizeof(struct sockaddr_in);
    printf("Waiting for incoming connections...\n");
    SOCKET client_socket = accept(server_socket, (struct sockaddr *)client, &client_len);
    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed. Error Code: %d\n", WSAGetLastError());
    } else {
        printf("Connection accepted.\n");
    }
    return client_socket;
}

int main() {
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;
    char buffer[BUFFER_SIZE];
    bool heartbeat_received = false;

    printf("Initializing Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Winsock initialized.\n");

    // Create socket
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket created.\n");

    // Prepare sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    // Bind
    if (bind(server_socket, (struct sockaddr *)&server, sizeof(server)) == SOCKET_ERROR) {
        printf("Bind failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Bind done.\n");

    // Listen
    listen(server_socket, 3);

    while (true) {
        client_socket = waitForClient(server_socket, &client);
        if (client_socket == INVALID_SOCKET) {
            break;
        }

        // Receive messages
        int recv_size;
        while ((recv_size = recv(client_socket, buffer, BUFFER_SIZE, 0)) > 0) {
            buffer[recv_size] = '\0';
            if (strcmp(buffer, HEARTBEAT_MESSAGE) == 0) {
                printf("Heartbeat received.\n");
                heartbeat_received = true;
            } else {
                printf("Message received: %s\n", buffer);
            }
        }

        if (recv_size == SOCKET_ERROR) {
            printf("Recv failed. Error Code: %d\n", WSAGetLastError());
        } else {
            printf("Client disconnected.\n");
        }

        // Close client socket
        closesocket(client_socket);
    }

    // Close server socket
    closesocket(server_socket);
    WSACleanup();

    return 0;
}
