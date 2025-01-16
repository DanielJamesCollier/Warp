#ifndef UTILS_H_
#define UTILS_H_

#include "base/core.h"

void
get_filename_without_extension(const char *input, char *output, size_t output_size);

const char *
getFileName(const char *fullPath);

void
convertToDoubleBackslashes(const char *input, char *output, size_t outputSize);

b8
create_app_data_folder(char *out, const char *firstFolder, ...);

#endif  // UTILS_H_
