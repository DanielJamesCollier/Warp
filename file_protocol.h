#ifndef FILE_TRANSFER_PROTOCOL_H
#define FILE_TRANSFER_PROTOCOL_H

#include "base/core.h"

#define PROTOCOL_MAGIC 0xAABBCCDD
#define MAX_FILENAME_LENGTH 260
#define MAX_CHUNK_SIZE 4096

typedef enum {
  MSG_TYPE_FILE_INIT,
  MSG_TYPE_FILE_CHUNK,
  MSG_TYPE_FILE_COMPLETE,
  MSG_TYPE_COMPILE,
} MessageType;

typedef enum {
  FILE_COMPILER,
  FILE_SOURCE_FILE,
  FILE_OBJ_FILE,
} FileType;

#pragma pack(push, 1)
typedef struct {
  u32 magic;
  u32 type;
  u64 length;
} ProtocolHeader;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  char filename[MAX_FILENAME_LENGTH];
  u64 filesize;
  u64 type;
} FileInitMessage;
#pragma pack(pop)

#pragma pack(push, 1)
typedef struct {
  u32 chunk_size;
  u8 chunk_data[MAX_CHUNK_SIZE];
} FileChunkMessage;
#pragma pack(pop)

#endif  // FILE_TRANSFER_PROTOCOL_H
