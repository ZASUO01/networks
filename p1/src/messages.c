#include "network.h"
#include "utils.h"
#include <arpa/inet.h>
#include <messages.h>
#include <string.h>
#include <sys/socket.h>

// make a valid frame
void make_frame(Frame *f, uint16_t id, uint8_t flags, const char *data,
                size_t data_size, int end_char) {
  // current frame size
  size_t frame_size = FRAME_HEADER_BYTES;

  // set sync bytes
  f->SYNC1 = htonl(SYNC_BYTES);
  f->SYNC2 = htonl(SYNC_BYTES);

  // init check sum with 0
  f->checksum = 0;

  // set length and data
  if (data_size > 0) {
    memcpy(f->data, data, data_size);
    frame_size += data_size;

    if (end_char > 0) {
      f->lenght = htons(data_size + 1);
      frame_size += 1;

      f->data[data_size] = '\n';
      f->data[data_size + 1] = '\0';
    } else {
      f->lenght = htons(data_size);
      f->data[data_size] = '\0';
    }
  } else {
    f->lenght = 0;
    f->data[0] = '\0';
  }

  // set id and flags
  f->id = htons(id);
  f->flags = flags;

  // set checksum
  f->checksum = get_checksum(f, frame_size);
  f->checksum = htons(f->checksum);
}

// check if a frame is valid
static int check_valid_frame(Frame *f) {
  LOG_MSG(LOG_INFO, "check_valid_frame(): init\n");

  // check sync bytes
  if (ntohl(f->SYNC1) != SYNC_BYTES || ntohl(f->SYNC2) != SYNC_BYTES) {
    LOG_MSG(LOG_ERROR, "check_valid_frame(): end with invalid sync\n");
    return -1;
  }

  uint16_t len = ntohs(f->lenght);
  uint16_t id = ntohs(f->id);

  // check length
  if (len > MAX_DATA_BYTES) {
    LOG_MSG(LOG_ERROR, "check_valid_frame(): end with invalid length: %d\n",
            len);
    return -1;
  }

  // check checksum
  Frame temp;

  if (len > 0) {
    char *data = malloc(len + 1);
    data = strdup(f->data);
    data[len] = '\0';

    if (data[len - 1] == '\n') {
      data[len - 1] = '\0';
      make_frame(&temp, id, f->flags, data, len - 1, 1);
    } else {
      make_frame(&temp, id, f->flags, data, len, 0);
    }

    free(data);
  } else {
    make_frame(&temp, id, f->flags, NULL, 0, 0);
  }

  if (f->checksum != temp.checksum) {
    LOG_MSG(LOG_ERROR,
            "check_valid_frame(): end with invalid checksum %d != %d\n",
            f->checksum, temp.checksum);
    return -1;
  }

  // valid frame
  LOG_MSG(LOG_INFO, "check_valid_frame(): end with valid frame\n");
  return 0;
}

// send a frame to server
int send_frame(int fd, Frame *f, size_t f_size) {
  LOG_MSG(LOG_INFO, "send_frame(): init\n");

  // try to send the frame
  ssize_t bytes_count = send(fd, f, f_size, 0);
  if (bytes_count != (ssize_t)f_size) {
    LOG_MSG(LOG_ERROR, "send_frame(): end with send failure\n");
    return -1;
  }

  LOG_MSG(LOG_INFO, "send_frame(): end with %ld bytes sent\n", bytes_count);
  return 0;
}

// receive bytes and check if its a frame
int receive_frame(int fd, Frame *f, size_t f_size) {
  LOG_MSG(LOG_INFO, "receive_frame(): init\n");

  // set bytes
  memset(f, 0, f_size);

  // try to receive frame
  ssize_t bytes_count = recv(fd, f, f_size, 0);

  // received bytes
  if (bytes_count > 0) {
    LOG_MSG(LOG_INFO, "%ld bytes received\n", bytes_count);

    // check if it's a valid frame
    if (check_valid_frame(f) != 0) {
      LOG_MSG(LOG_ERROR, "receive_frame(): end with invalid receipt\n");
      return -1;
    }

    LOG_MSG(LOG_INFO, "receive_frame(): end with valid receipt\n");
    return 0;
  }

  // no response
  LOG_MSG(LOG_ERROR, "receive_frame(): end with receipt failure\n");
  return -1;
}
