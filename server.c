#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_protocol.h"

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 6101)
#include <ws2tcpip.h>
#pragma warning(pop)
#include <shlobj.h> // For SHGetFolderPath
#include <windows.h>
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Shell32.lib")

#define PORT 8080
#define BUFFER_SIZE 1024

// Function to create the program folder and return the folder path
char* createProgramFolder(const char* folderName)
{
    static char programFolderPath[MAX_PATH]; // Static to retain value after
                                             // function returns
    char localAppDataPath[MAX_PATH];

    // Get the path to %LOCALAPPDATA%
    if (SUCCEEDED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppDataPath))) {
        // Create a path for the program's folder
        snprintf(programFolderPath, MAX_PATH, "%s\\%s", localAppDataPath, folderName);

        // Create the directory if it doesn't exist
        if (CreateDirectory(programFolderPath, NULL) || GetLastError() == ERROR_ALREADY_EXISTS) {
            printf("Program folder created or already exists: %s\n", programFolderPath);
            return programFolderPath; // Return the path of the created folder
        } else {
            printf("Failed to create program folder. Error: %lu\n", GetLastError());
            return NULL; // Failure, folder not created
        }
    } else {
        printf("Failed to get LocalAppData path.\n");
        return NULL; // Failure to get path
    }
}

// Function to wait for a client connection
SOCKET
waitForClient(SOCKET server_socket, struct sockaddr_in* client)
{
    int client_len = sizeof(struct sockaddr_in);
    printf("Waiting for incoming connections...\n");
    SOCKET client_socket = accept(server_socket, (struct sockaddr*)client, &client_len);
    if (client_socket == INVALID_SOCKET) {
        printf("Accept failed. Error Code: %d\n", WSAGetLastError());
    } else {
        printf("Connection accepted.\n");
    }
    return client_socket;
}

// Function to handle file transfer from client
void handleFileTransfer(SOCKET client_socket, const char* folderPath)
{
    ProtocolHeader header;
    FileInitMessage init_msg;
    FileChunkMessage chunk_msg;

    // Receive ProtocolHeader. Then check the header.type  to determine the next message read by recv should be MSG_TYPE_FILE_INIT.
    if (recv(client_socket, (char*)&header, sizeof(header), 0) <= 0 || header.magic != PROTOCOL_MAGIC || header.type != MSG_TYPE_FILE_INIT) {
        printf("Error receiving file initialization message.\n");
        return;
    }
    // Read FileInitMessage.
    recv(client_socket, (char*)&init_msg, sizeof(init_msg), 0);

    printf("Receiving file: %s, Size: %llu bytes\n", init_msg.filename, init_msg.filesize);

    // Create the full file path in the designated folder
    char filePath[MAX_PATH];
    snprintf(filePath, MAX_PATH, "%s\\%s", folderPath, init_msg.filename);

    // Open the file to write to
    FILE* file = fopen(filePath, "wb");
    if (!file) {
        printf("Failed to open file for writing: %s\n", filePath);
        return;
    }

    // Receive and write file data in chunks
    size_t total_received = 0;
    while (total_received < init_msg.filesize) {

        // TODO: add support for partial reads.
        if (recv(client_socket, (char*)&chunk_msg, sizeof(chunk_msg), 0) <= 0) {
            printf("Error receiving file chunk.\n");
            fclose(file);
            return;
        }

        fwrite(chunk_msg.chunk_data, 1, chunk_msg.chunk_size, file);
        total_received += chunk_msg.chunk_size;
        printf("Received chunk %zu/%llu bytes\n", total_received, init_msg.filesize);
    }

    // Receive FileCompleteMessage
    recv(client_socket, (char*)&header, sizeof(header), 0);
    if (header.type != MSG_TYPE_FILE_COMPLETE) {
        printf("Error receiving file complete message.\n");
    } else {
        printf("File transfer complete.\n");
    }

    fclose(file);
}

int main()
{
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;

    // Create the program folder under LocalAppData and get the folder path
    char* folderPath = createProgramFolder("warp");
    if (!folderPath) {
        return 1; // Exit if folder creation failed
    }

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

    // Bind the socket
    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        int err_code = WSAGetLastError();
        printf("Bind failed. Error Code: %d\n", err_code);

        // Handle different error codes
        switch (err_code) {
        case WSAEADDRINUSE:
            printf("Error: The address is already in use.\n");
            break;
        case WSAEADDRNOTAVAIL:
            printf("Error: The specified address is not available.\n");
            break;
        case WSAEACCES:
            printf("Error: Permission denied (you may need administrator "
                   "privileges).\n");
            break;
        case WSAEAFNOSUPPORT:
            printf("Error: The address family is not supported.\n");
            break;
        case WSAEINVAL:
            printf("Error: The socket is already bound to a different address.\n");
            break;
        default:
            printf("Unknown error.\n");
            break;
        }
        exit(EXIT_FAILURE);
    } else {
        printf("Bind done.\n");
    }

    // Listen
    listen(server_socket, 3);

    while (true) {
        client_socket = waitForClient(server_socket, &client);
        if (client_socket == INVALID_SOCKET) {
            break;
        }

        // Handle file transfer to the folder created in %LOCALAPPDATA%\warp
        handleFileTransfer(client_socket, folderPath);

        // Close client socket
        closesocket(client_socket);
    }

    // Close server socket
    closesocket(server_socket);
    WSACleanup();

    return 0;
}
