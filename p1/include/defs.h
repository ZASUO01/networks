// file:        defs.h
// description: definitions for the constants and main structs used by the
// programs
#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

// byte sizes
#define SYNC_BYTES 0xDCC023C2
#define FRAME_HEADER_BYTES 15
#define MAX_DATA_BYTES 4096
#define END_CHAR_BYTE 1

// frame flags
#define ACKNOWLEDGE_FLAG 0x80
#define END_FLAG 0x40
#define RESET_FLAG 0x20
#define NO_FLAGS 0x00

// special ids
#define RESET_ID 0xFFFF

// transmission parameters
#define MAX_ATTEMPTS 16

#pragma pack(1)

// packet frame
typedef struct {
  uint32_t SYNC1;
  uint32_t SYNC2;
  uint16_t checksum;
  uint16_t lenght;
  uint16_t id;
  uint8_t flags;
  char data[MAX_DATA_BYTES];
} Frame;

#pragma pack(0)

#endif
