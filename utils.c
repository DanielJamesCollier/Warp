#include "utils.h"

#define _CRT_SECURE_NO_WARNINGS

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#if defined(_WIN32)
#pragma warning(push)
#pragma warning(disable : 6101)
#include <ws2tcpip.h>
#pragma warning(pop)
#include <Windows.h>
#include <shlobj.h>  // For SHGetFolderPath
#endif

#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "Shell32.lib")

void get_filename_without_extension(const char* input, char* output, size_t output_size) {
  if (input == NULL || output == NULL) {
    return;
  }

  // Find the last path separator
  const char* last_separator = strrchr(input, '/');
  if (!last_separator) {
    last_separator = strrchr(input, '\\');  // Check for Windows-style separators
  }

  const char* filename = last_separator ? last_separator + 1 : input;  // Start after the separator

  // Find the last dot in the filename
  const char* last_dot = strrchr(filename, '.');
  size_t length = last_dot ? (size_t)(last_dot - filename) : strlen(filename);

  // Copy the result into the output buffer, ensuring no overflow
  if (length < output_size) {
    strncpy(output, filename, length);
    output[length] = '\0';  // Null-terminate
  } else {
    strncpy(output, filename, output_size - 1);
    output[output_size - 1] = '\0';
  }
}

const char* getFileName(const char* fullPath) {
  if (fullPath == NULL) {
    return NULL;
  }

  // Find the last occurrence of '/' or '\' in the path
  const char* lastSlash = strrchr(fullPath, '/');
  const char* lastBackslash = strrchr(fullPath, '\\');

  // Use the separator that appears last in the string
  const char* lastSeparator = (lastSlash > lastBackslash) ? lastSlash : lastBackslash;

  // If no separator is found, the full path is already a file name
  return lastSeparator ? lastSeparator + 1 : fullPath;
}

void convertToDoubleBackslashes(const char* input, char* output, size_t outputSize) {
  size_t inputLength = strlen(input);
  size_t j = 0;

  for (size_t i = 0; i < inputLength; i++) {
    if (j >= outputSize - 1) {  // Prevent buffer overflow
      fprintf(stderr, "Output buffer too small\n");
      return;
    }

    if (input[i] == '\\' || input[i] == '/') {
      // Add two backslashes for each single backslash or forward slash
      if (j < outputSize - 2) {
        output[j++] = '\\';
        output[j++] = '\\';
      } else {
        fprintf(stderr, "Output buffer too small\n");
        return;
      }
    } else {
      // Copy the character as is
      output[j++] = input[i];
    }
  }
  output[j] = '\0';  // Null-terminate the string
}

bool create_app_data_folder(char* out, const char* firstFolder, ...) {
  // Get the path to %LOCALAPPDATA%
  if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, out))) {
    printf("Failed to get AppData folder path.\n");
    return false;
  }

  // Create the "Warp" directory if it doesn't exist
  if (!CreateDirectory(out, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
    printf("Failed to create Warp directory. Path: %s\n", out);
    return false;
  }

  // Process the variable arguments for subfolders
  va_list args;
  va_start(args, firstFolder);

  const char* folder = firstFolder;
  while (folder != NULL) {
    // Append the next folder to the path
    strcat(out, "\\");
    strcat(out, folder);

    // Create the directory
    if (!CreateDirectory(out, NULL) && GetLastError() != ERROR_ALREADY_EXISTS) {
      printf("Failed to create directory. Path: %s\n", out);
      va_end(args);
      return false;
    }

    // Get the next argument
    folder = va_arg(args, const char*);
  }

  va_end(args);

  return true;
}
