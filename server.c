#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file_protocol.h"
#include "utils.h"

#include <windows.h>

#define PORT 8080
#define BUFFER_SIZE 1024

static char warp_dir[MAX_PATH];
static char servers_dir[MAX_PATH];
static char sources_dir[MAX_PATH];
static char artifacts_dir[MAX_PATH];
static char clang_path[MAX_PATH];

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
void handleFileTransfer(SOCKET client_socket)
{
    ProtocolHeader header;
    FileInitMessage init_msg;
    FileChunkMessage chunk_msg;

    recv(client_socket, (char*)&init_msg, sizeof(init_msg), 0);
    printf("Receiving file: %s, Size: %llu bytes\n", init_msg.filename, init_msg.filesize);

    // Create the full file path in the designated folder
    char filePath[MAX_PATH];
    snprintf(filePath, MAX_PATH, "%s\\%s", servers_dir, init_msg.filename);

    if (init_msg.type == FILE_COMPILER) {
        strcpy(clang_path, filePath);
    }

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

void get_filename_without_extension(const char* input, char* output, size_t output_size)
{
    if (input == NULL || output == NULL) {
        return;
    }

    // Find the last path separator
    const char* last_separator = strrchr(input, '/');
    if (!last_separator) {
        last_separator = strrchr(input, '\\'); // Check for Windows-style separators
    }

    const char* filename = last_separator ? last_separator + 1 : input; // Start after the separator

    // Find the last dot in the filename
    const char* last_dot = strrchr(filename, '.');
    size_t length = last_dot ? (size_t)(last_dot - filename) : strlen(filename);

    // Copy the result into the output buffer, ensuring no overflow
    if (length < output_size) {
        strncpy(output, filename, length);
        output[length] = '\0'; // Null-terminate
    } else {
        strncpy(output, filename, output_size - 1);
        output[output_size - 1] = '\0';
    }
}

void compile(SOCKET client_socket)
{
    puts("compiling...");

    FileInitMessage init_msg;
    recv(client_socket, (char*)&init_msg, sizeof(init_msg), 0);

    if (init_msg.type != FILE_SOURCE_FILE) {
        puts("init_msg.type != FILE_SOURCE_FILE");
        exit(EXIT_FAILURE);
    }

    char filePath[MAX_PATH];
    snprintf(filePath, MAX_PATH, "%s\\%s", sources_dir, init_msg.filename);
    FILE* file = fopen(filePath, "wb");
    if (!file) {
        printf("Failed to open file for writing: %s\n", filePath);
        return;
    }

    // Receive and write file data in chunks
    FileChunkMessage chunk_msg;
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
    ProtocolHeader header;
    recv(client_socket, (char*)&header, sizeof(header), 0);
    if (header.type != MSG_TYPE_FILE_COMPLETE) {
        printf("Error receiving file complete message.\n");
    } else {
        printf("File transfer complete.\n");
    }

    fclose(file);

    // Initialize structures for process information
    STARTUPINFO si = { sizeof(STARTUPINFO) }; // Zero-initialize and set the size
    PROCESS_INFORMATION pi = { 0 }; // Zero-initialize
                                    //
    printf("clang_path=%s\n", clang_path);
    printf("path=%s\n", filePath);

    char fixed_path[MAX_PATH];
    convertToDoubleBackslashes(filePath, fixed_path, MAX_PATH);

    // TODO arena alloc this.
    char command[512];
    // -c because we only want object files.
    // /Fo to specify the output location.
    char file_name_no_extension[MAX_PATH];
    get_filename_without_extension(init_msg.filename, file_name_no_extension, MAX_PATH);
    snprintf(command, 512, "%s -c %s /Fo%s\\%s.obj", clang_path, fixed_path, artifacts_dir, file_name_no_extension);

    printf("new_path=%s\n", fixed_path);

    // Compile.
    if (CreateProcess(
            NULL,
            command,
            NULL, // Process security attributes
            NULL, // Thread security attributes
            FALSE, // Inherit handles
            0, // Creation flags (e.g., CREATE_NEW_CONSOLE)
            NULL, // Environment (NULL = use parent's environment)
            NULL, // Current directory (NULL = use parent's directory)
            &si, // Startup information
            &pi // Process information
            )) {
        printf("Process launched successfully!\n");
        // Wait for the process to complete (optional)
        WaitForSingleObject(pi.hProcess, INFINITE);

        // Clean up handles
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    } else {
        // Error handling
        printf("Failed to launch process. Error: %lu\n", GetLastError());
    }

    // need command line flags.
    // need path to compiler.
    // store the file to Local/warp/source_cache
    // send back the obj
}

int handle_command(SOCKET client_socket)
{
    ProtocolHeader header;

    // Receive ProtocolHeader. Then check the header.type  to determine the next message read by recv should be MSG_TYPE_FILE_INIT.
    int result = recv(client_socket, (char*)&header, sizeof(header), 0);
    if (result == 0) {
        // Graceful disconnection by the remote peer
        printf("Socket disconnected gracefully.\n");
        return -1;
    }

    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        if (error == WSAECONNRESET) {
            // Connection reset by peer
            printf("Socket disconnected forcibly.\n");
            return -1;
        } else {
            printf("Socket error: %d\n", error);
            return -1;
        }
    }

    if (header.magic != PROTOCOL_MAGIC) {
        printf("header does not contain magic.\n");
        return -1;
    }

    switch (header.type) {
    case MSG_TYPE_FILE_INIT: {
        handleFileTransfer(client_socket);
        break;
    }
    case MSG_TYPE_COMPILE: {
        compile(client_socket);
        break;
    }
    default: {
        puts("unsupported command.");
    }
    }
    return 0;
}

int main()
{
    // Local\\warp
    bool ok = create_app_data_folder(warp_dir, "Warp", NULL);
    if (!ok) {
        printf("Failed to create the warp folder.");
        return 1;
    }

    // Local\\server
    ok = create_app_data_folder(servers_dir, "Warp", "server", NULL);
    if (!ok) {
        printf("Failed to create the warp server folder.");
        return 1;
    }

    // Local\\server\\sources
    ok = create_app_data_folder(sources_dir, "Warp", "server", "sources", NULL);
    if (!ok) {
        printf("Failed to create the warp server folder.");
        return 1;
    }

    // Local\\server\\artifacts
    ok = create_app_data_folder(artifacts_dir, "Warp", "server", "artifacts", NULL);
    if (!ok) {
        printf("Failed to create the warp server folder.");
        return 1;
    }

    printf("Initializing Winsock...\n");
    WSADATA wsa;
    SOCKET server_socket, client_socket;
    struct sockaddr_in server, client;

    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Failed. Error Code: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Winsock initialized.\n");

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Could not create socket: %d\n", WSAGetLastError());
        return 1;
    }

    printf("Socket created.\n");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(PORT);

    if (bind(server_socket, (struct sockaddr*)&server, sizeof(server)) == SOCKET_ERROR) {
        int err_code = WSAGetLastError();
        printf("Bind failed. Error Code: %d\n", err_code);

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

    listen(server_socket, 3);

reconnect:
    client_socket = waitForClient(server_socket, &client);
    if (client_socket == INVALID_SOCKET) {
        puts("INVALID_SOCKET");
        return EXIT_FAILURE;
    }

    while (true) {
        puts("Waiting for next command...");
        if (handle_command(client_socket) == -1) {
            goto reconnect;
        }
    }

    closesocket(server_socket);
    WSACleanup();

    return 0;
}
