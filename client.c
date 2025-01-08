#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <windows.h>
#include "file_protocol.h" // Include the protocol header

#pragma comment(lib, "ws2_32.lib")

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define CMD_BUFFER_SIZE 128

void send_file(SOCKET client_socket, const char *file_path) {
    // Extract the filename from the full path
    const char *filename = strrchr(file_path, '\\');  // For Windows paths
    if (!filename) {
        filename = strrchr(file_path, '/');  // For Unix-like paths
    }
    filename = (filename) ? filename + 1 : file_path;  // Move past the slash or use the full path if no slash found

    FILE *file = fopen(file_path, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", file_path);
        return;
    }

    // Send FileInitMessage
    FileInitMessage init_msg;
    fseek(file, 0, SEEK_END);
    init_msg.filesize = ftell(file);  // Corrected field name (was file_size)
    rewind(file);
    strncpy(init_msg.filename, filename, sizeof(init_msg.filename) - 1); // Only copy the filename, not the full path
    init_msg.filename[sizeof(init_msg.filename) - 1] = '\0';

    // Send ProtocolHeader first, followed by FileInitMessage
    ProtocolHeader header = {PROTOCOL_MAGIC, MSG_TYPE_FILE_INIT, sizeof(init_msg)};
    send(client_socket, (char *)&header, sizeof(header), 0);
    send(client_socket, (char *)&init_msg, sizeof(init_msg), 0);

    // Send file data in chunks
    FileChunkMessage chunk_msg;
    size_t bytes_read;
    while ((bytes_read = fread(chunk_msg.chunk_data, 1, sizeof(chunk_msg.chunk_data), file)) > 0) {
        chunk_msg.chunk_size = (uint32_t)bytes_read;

        // Set the header for the chunk message
        header.type = MSG_TYPE_FILE_CHUNK;
        header.length = sizeof(chunk_msg.header) + bytes_read; // Adjust length

        send(client_socket, (char *)&header, sizeof(header), 0);
        send(client_socket, (char *)&chunk_msg, sizeof(chunk_msg.header) + bytes_read, 0);
    }

    // Send FileCompleteMessage
    header.type = MSG_TYPE_FILE_COMPLETE;
    header.length = 0; // No additional data for FileCompleteMessage
    send(client_socket, (char *)&header, sizeof(header), 0);

    fclose(file);
    printf("File transfer complete.\n");
}

int main() {
    WSADATA wsa;
    SOCKET client_socket;
    struct sockaddr_in server;
    char command[CMD_BUFFER_SIZE];

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

    // Command loop
    while (1) {
        printf("Enter a command (SEND_FILE <filepath>, QUIT): ");
        if (!fgets(command, CMD_BUFFER_SIZE, stdin)) {
            printf("Error reading command.\n");
            break;
        }

        // Remove newline character
        command[strcspn(command, "\n")] = 0;

        if (strncmp(command, "SEND_FILE ", 10) == 0) {
            const char *file_path = command + 10;
            send_file(client_socket, file_path);
        } else if (strcmp(command, "QUIT") == 0) {
            printf("Exiting client.\n");
            break;
        } else {
            printf("Unknown command. Try SEND_FILE <filepath> or QUIT.\n");
        }
    }

    // Close socket
    closesocket(client_socket);

    WSACleanup();

    return 0;
}
