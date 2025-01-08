#ifndef FILE_TRANSFER_PROTOCOL_H
#define FILE_TRANSFER_PROTOCOL_H

#include <stdint.h>

#define PROTOCOL_MAGIC 0xAABBCCDD  // Unique identifier for protocol messages

// Maximum sizes
#define MAX_FILENAME_LENGTH 256
#define MAX_CHUNK_SIZE 1024

// Message types
typedef enum {
  MSG_TYPE_HEARTBEAT = 1,
  MSG_TYPE_FILE_INIT,     // File transmission initialization
  MSG_TYPE_FILE_CHUNK,    // File chunk
  MSG_TYPE_FILE_COMPLETE  // File transmission completion
} MessageType;

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
  ProtocolHeader header;
  char filename[MAX_FILENAME_LENGTH];  // Name of the file to be sent
  uint64_t filesize;                   // Total size of the file
} FileInitMessage;
#pragma pack(pop)

// File chunk message
#pragma pack(push, 1)
typedef struct {
  ProtocolHeader header;
  uint32_t chunk_size;                 // Size of the chunk being sent
  uint8_t chunk_data[MAX_CHUNK_SIZE];  // File chunk data
} FileChunkMessage;
#pragma pack(pop)

// File transmission completion message
#pragma pack(push, 1)
typedef struct {
  ProtocolHeader header;
} FileCompleteMessage;
#pragma pack(pop)

#endif  // FILE_TRANSFER_PROTOCOL_H
