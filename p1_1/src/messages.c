#include "defs.h"
#include "network.h"
#include "utils.h"
#include <arpa/inet.h>
#include <messages.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

// init a frame packet
void set_frame(Frame *f, const char *data, size_t data_size) {
  // prevent buffer overflow
  if (data_size + 1 > MAX_DATA_BYTES) {
    log_exit("Data size greater than max size");
  }

  // initialize frame memory
  memset(f, 0, sizeof(Frame));

  // set frame fields
  f->SYNC1 = SYNC_BYTES;
  f->SYNC2 = SYNC_BYTES;
  f->checksum = 0;
  f->lenght = data_size + 1;
  f->id = 0;
  f->flags = NO_FLAGS;

  // copy data to the frame
  memcpy(f->data, data, data_size);
  f->data[data_size] = '\n';
  f->data[data_size + 1] = '\0';

  // get checksum
  size_t total_frame_size = FRAME_BYTES + data_size + 1;
  f->checksum = get_md5_checksum(f, total_frame_size);
  printf("%x %d\n", f->checksum, f->checksum);
}

// convert frame fields to big endian format
void to_big_endian(Frame *f) {
  f->SYNC1 = htonl(f->SYNC1);
  f->SYNC2 = htonl(f->SYNC2);
  f->checksum = htons(f->checksum);
  f->lenght = htons(f->lenght);
  f->id = htons(f->id);
  f->flags = htons(f->flags);
}

void send_receive(int fd, void *request, int req_size, void *response,
                  int res_size) {
  ssize_t bytes_count = send(fd, request, req_size, 0);
  if (bytes_count != req_size) {
    log_exit("Send Failure");
  }
}
