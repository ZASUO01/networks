#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>
#include <stdlib.h>

// byte sizes
#define SYNC_BYTES 0xDCC023C2
#define FRAME_BYTES 15
#define MAX_DATA_BYTES 4096

// frame flags
#define NO_FLAGS 0x3f
#define RESET_FLAG 0x20
#define END_FLAG 0x40
#define ACKNOWLEDGE_FLAG 0x80

// Packet Frame
#pragma pack(1)
typedef struct {
  uint32_t SYNC1;
  uint32_t SYNC2;
  uint16_t checksum;
  uint16_t lenght;
  uint16_t id;
  uint8_t flags;
  uint8_t data[MAX_DATA_BYTES];
} Frame;
#pragma pack(0)

#endif
