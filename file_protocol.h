#ifndef FILE_TRANSFER_PROTOCOL_H
#define FILE_TRANSFER_PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_MAGIC 0xAABBCCDD  // Unique identifier for protocol messages

// Maximum sizes
#define MAX_FILENAME_LENGTH 256
#define MAX_CHUNK_SIZE 4096

// Message types
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

// General protocol message header
#pragma pack(push, 1)  // Ensure no padding
typedef struct {
  uint32_t magic;   // Protocol magic number
  uint32_t type;    // Message type (MessageType)
  uint64_t length;  // Length of the payload (excluding header)
} ProtocolHeader;
#pragma pack(pop)

// File initialization message
#pragma pack(push, 1)
typedef struct {
  char filename[MAX_FILENAME_LENGTH];  // Name of the file to be sent
  uint64_t filesize;                   // Total size of the file
  uint64_t type;                       // FileType
} FileInitMessage;
#pragma pack(pop)

// File chunk message
#pragma pack(push, 1)
typedef struct {
  uint32_t chunk_size;                 // Size of the chunk being sent
  uint8_t chunk_data[MAX_CHUNK_SIZE];  // File chunk data
} FileChunkMessage;
#pragma pack(pop)

#endif  // FILE_TRANSFER_PROTOCOL_H
