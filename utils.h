#pragma once

#include <stdbool.h>

void convertToDoubleBackslashes(const char* input, char* output, size_t outputSize);
bool create_app_data_folder(char* out, const char* firstFolder, ...);
