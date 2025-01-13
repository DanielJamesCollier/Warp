#define _CRT_SECURE_NO_WARNINGS
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 6101)
#include <ws2tcpip.h>
#pragma warning(pop)
#include <windows.h>
#endif

#include "file_protocol.h"
#include "utils.h"

#define SERVER_IP "127.0.0.1"
#define PORT 8080
#define CMD_BUFFER_SIZE 128

static char warp_dir[MAX_PATH];
static char servers_dir[MAX_PATH];
static char sources_dir[MAX_PATH];
static char artifacts_dir[MAX_PATH];
static char clang_path[MAX_PATH];

void sync_compiler(SOCKET client_socket, const char* file_path) {
  strcpy(clang_path, file_path);

  // Extract the filename from the full path
  const char* filename = strrchr(file_path, '\\');  // For Windows paths
  if (!filename) {
    filename = strrchr(file_path, '/');  // For Unix-like paths
  }
  filename = (filename) ? filename + 1 : file_path;  // Move past the slash or use the
                                                     // full path if no slash found

  FILE* file = fopen(file_path, "rb");
  if (!file) {
    printf("Failed to open file: %s\n", file_path);
    return;
  }

  FileInitMessage payload;
  payload.type = FILE_COMPILER;
  {
    fseek(file, 0, SEEK_END);
    payload.filesize = ftell(file);
    rewind(file);

    // Only copy the filename, not the full path
    strncpy(payload.filename, filename, sizeof(payload.filename) - 1);
    payload.filename[sizeof(payload.filename) - 1] = '\0';
  }

  ProtocolHeader header = {PROTOCOL_MAGIC, MSG_TYPE_FILE_INIT, sizeof(payload)};

  send(client_socket, (char*)&header, sizeof(header), 0);
  send(client_socket, (char*)&payload, sizeof(payload), 0);

  // Send file data in chunks
  FileChunkMessage chunk_msg;
  size_t bytes_read;
  while ((bytes_read = fread(chunk_msg.chunk_data, 1, sizeof(chunk_msg.chunk_data), file)) > 0) {
    chunk_msg.chunk_size = (uint32_t)bytes_read;
    send(client_socket, (char*)&chunk_msg, sizeof(chunk_msg), 0);
  }

  // Send FileCompleteMessage
  header.type = MSG_TYPE_FILE_COMPLETE;
  header.length = 0;  // No additional data for FileCompleteMessage
  printf("sending %zi\n", sizeof(header));
  send(client_socket, (char*)&header, sizeof(header), 0);
  fclose(file);
  printf("File transfer complete.\n");
}

void send_compile_command(SOCKET client_socket, const char* file_path) {
  FILE* file = fopen(file_path, "rb");
  if (!file) {
    printf("Failed to open file: %s\n", file_path);
    return;
  }

  FileInitMessage payload = {0};
  payload.type = FILE_SOURCE_FILE;

  fseek(file, 0, SEEK_END);
  payload.filesize = ftell(file);
  rewind(file);

  const char* simple_name = getFileName(file_path);
  strcpy(payload.filename, simple_name);

  ProtocolHeader header = {PROTOCOL_MAGIC, MSG_TYPE_COMPILE, sizeof(payload)};

  send(client_socket, (char*)&header, sizeof(header), 0);
  send(client_socket, (char*)&payload, sizeof(payload), 0);

  // Send file data in chunks
  FileChunkMessage chunk_msg;
  size_t bytes_read;
  while ((bytes_read = fread(chunk_msg.chunk_data, 1, sizeof(chunk_msg.chunk_data), file)) > 0) {
    chunk_msg.chunk_size = (uint32_t)bytes_read;
    send(client_socket, (char*)&chunk_msg, sizeof(chunk_msg), 0);
  }

  // Send FileCompleteMessage
  header.type = MSG_TYPE_FILE_COMPLETE;
  header.length = 0;  // No additional data for FileCompleteMessage
  printf("sending %zi\n", sizeof(header));
  send(client_socket, (char*)&header, sizeof(header), 0);
  fclose(file);
  printf("File transfer complete.\n");
}

const char* preprocess(const char* path) {
  static char preprocessed_file[MAX_PATH];
  static char outputBuffer[4096];  // Buffer to hold stdout content

  // TODO parse out file name only.
  memset(preprocessed_file, 0, sizeof(preprocessed_file));
  strcat(preprocessed_file, sources_dir);
  strcat(preprocessed_file, "\\warp_pp_");
  strcat(preprocessed_file, path);

  // Create pipes for redirecting stdout
  HANDLE hReadPipe, hWritePipe;
  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};

  if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0)) {
    printf("Error creating pipe\n");
    return NULL;
  }

  // Ensure the write handle to the pipe is not inherited
  SetHandleInformation(hReadPipe, HANDLE_FLAG_INHERIT, 0);

  // Prepare the command line for clang-cl
  char command[512];
  snprintf(command, sizeof(command), "\"%s\" -E \"%s\"", clang_path, path);

  // Setup the structures for CreateProcess
  STARTUPINFO si = {sizeof(STARTUPINFO)};
  PROCESS_INFORMATION pi = {0};

  si.dwFlags |= STARTF_USESTDHANDLES;
  si.hStdOutput = hWritePipe;  // Redirect stdout to the write end of the pipe
  si.hStdError = hWritePipe;   // Redirect stderr to the same pipe
  si.hStdInput = NULL;

  // Create the process
  if (CreateProcess(NULL,     // Application name
                    command,  // Command line
                    NULL,     // Process handle not inheritable
                    NULL,     // Thread handle not inheritable
                    TRUE,     // Set handle inheritance to TRUE
                    0,        // No creation flags
                    NULL,     // Use parent's environment block
                    NULL,     // Use parent's starting directory
                    &si,      // Pointer to STARTUPINFO structure
                    &pi)      // Pointer to PROCESS_INFORMATION structure
  ) {
    // Close the write end of the pipe in this process
    CloseHandle(hWritePipe);

    // Read the output from the child process
    DWORD bytesRead;
    FILE* tempFile = fopen(preprocessed_file, "w");
    if (!tempFile) {
      printf("Error opening temp file for writing\n");
      return NULL;
    }

    while (ReadFile(hReadPipe, outputBuffer, sizeof(outputBuffer) - 1, &bytesRead, NULL) && bytesRead > 0) {
      outputBuffer[bytesRead] = '\0';  // Null-terminate the string
      fprintf(tempFile, "%s", outputBuffer);
    }

    fclose(tempFile);

    // Wait for the process to finish
    WaitForSingleObject(pi.hProcess, INFINITE);

    // Clean up handles
    CloseHandle(hReadPipe);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    // Return the temporary file path
    return preprocessed_file;
  } else {
    printf("CreateProcess failed (%lu).\n", GetLastError());
    CloseHandle(hReadPipe);
    CloseHandle(hWritePipe);
    return NULL;
  }
}

int main() {
  // Local\\warp
  bool ok = create_app_data_folder(warp_dir, "Warp", NULL);
  if (!ok) {
    printf("Failed to create the warp folder.");
    return 1;
  }

  // Local\\server
  ok = create_app_data_folder(servers_dir, "Warp", "client", NULL);
  if (!ok) {
    printf("Failed to create the warp server folder.");
    return 1;
  }

  // Local\\server\\sources
  ok = create_app_data_folder(sources_dir, "Warp", "client", "preprocess", NULL);
  if (!ok) {
    printf("Failed to create the warp server folder.");
    return 1;
  }

  printf("Initializing Winsock...\n");
  WSADATA wsa;
  SOCKET client_socket;
  struct sockaddr_in server;
  char command[CMD_BUFFER_SIZE];
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
  if (connect(client_socket, (struct sockaddr*)&server, sizeof(server)) < 0) {
    printf("Connect failed. Error Code: %d\n", WSAGetLastError());
    return 1;
  }

  printf("Connected to server.\n");

  // Command loop
  while (1) {
    printf("Enter a command (sync_compiler <filepath>, compile <filepath>, exit): ");
    if (!fgets(command, CMD_BUFFER_SIZE, stdin)) {
      printf("Error reading command.\n");
      break;
    }

    // Remove newline character
    command[strcspn(command, "\n")] = 0;

    // TODO: harden.
    // TODO sync win32 GetEnvironmentStrings() then pass that to the remote compiler.
    if (strncmp(command, "sync_compiler", 13) == 0) {
      const char* file_path = command + 14;
      sync_compiler(client_socket, file_path);
    } else if (strncmp(command, "compile", 7) == 0) {
      printf("compiling.\n");
      const char* file_path = command + 8;
      const char* output = preprocess(file_path);

      // todo harden. getfilename can return null.
      send_compile_command(client_socket, output);
    } else if (strcmp(command, "exit") == 0) {
      printf("Exiting client.\n");
      break;
    } else {
      printf("Unknown command. Try send <filepath> or exit.\n");
    }
  }

  // Close socket
  closesocket(client_socket);

  WSACleanup();

  return 0;
}
