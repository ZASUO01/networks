#include "messages.h"
#include "defs.h"
#include "network.h"
#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

MsgControl msg_control;

// helper function to print a Frame
void print_frame(Frame *f) {
  printf("%x %x\n", f->SYNC1, f->SYNC2);
  printf("%d\n", f->checksum);
  printf("%d\n", f->lenght);
  printf("%d\n", f->id);
  printf("%d\n", f->flags);
  printf("%s", f->data);
}

// create a valid frame
void make_frame(Frame *f, uint16_t id, uint8_t flags, const char *data,
                size_t data_size) {
  size_t frame_size = FRAME_HEADER_BYTES;

  // initialize memory block with 0
  memset(f, 0, FRAME_HEADER_BYTES + MAX_DATA_BYTES);

  // set header fields in bi endian format
  f->SYNC1 = htonl(SYNC_BYTES);
  f->SYNC2 = htonl(SYNC_BYTES);
  f->checksum = 0;

  if (data_size > 0) {
    f->lenght = htons(data_size + 1);
    memcpy(f->data, data, data_size);
    f->data[data_size] = '\n';
    f->data[data_size + 1] = '\0';
    frame_size += data_size + 1;
  } else {
    f->lenght = 0;
  }

  f->id = htons(id);
  f->flags = flags;

  // get checksum from frame bytes and set it
  f->checksum = get_checksum(f, frame_size);
  f->checksum = htons(f->checksum);
}
