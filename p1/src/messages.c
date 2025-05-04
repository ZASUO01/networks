#include "defs.h"
#include "logger.h"
#include "network.h"
#include <arpa/inet.h>
#include <messages.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
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
    } else {
      f->lenght = htons(data_size);
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
  // frame variables to little-endian
  uint32_t _sync1 = ntohl(f->SYNC1);
  uint32_t _sync2 = ntohl(f->SYNC2);
  uint16_t _id = ntohs(f->id);
  uint16_t _lenght = ntohs(f->lenght);
  uint16_t _checksum = ntohs(f->checksum);

  LOG_MSG(LOG_INFO, "check_valid_frame(id = %hd): start", _id);

  // check sync bytes
  if (_sync1 != SYNC_BYTES || _sync2 != SYNC_BYTES) {
    LOG_MSG(LOG_ERROR, "check_valid_frame(id = %hd): invalid sync bytes %x %x",
            _id, _sync1, _sync2);
    return -1;
  }

  // check length
  if (_lenght > MAX_DATA_BYTES) {
    LOG_MSG(LOG_ERROR, "check_valid_frame(id = %hd): invalid data size", _id);
    return -1;
  }

  // make a temp frame to compare checksum
  Frame temp;

  if (_lenght > 0) {
    char *data = malloc(_lenght + 1);
    data = strdup(f->data);
    data[_lenght] = '\0';

    if (data[_lenght - 1] == '\n') {
      data[_lenght - 1] = '\0';
      make_frame(&temp, _id, f->flags, data, _lenght - 1, 1);
    } else {
      make_frame(&temp, _id, f->flags, data, _lenght, 0);
    }

    free(data);
  } else {
    make_frame(&temp, _id, f->flags, NULL, 0, 0);
  }

  uint16_t tmp_checksum = ntohs(temp.checksum);
  if (_checksum != tmp_checksum) {
    LOG_MSG(LOG_ERROR,
            "check_valid_frame(id = %hd): invalid checksum %hd != %hd", _id,
            _checksum, tmp_checksum);
    return -1;
  }

  // valid frame
  LOG_MSG(LOG_INFO, "check_valid_frame(id = %hd): complete", _id);
  return 0;
}

// send a frame to server
int send_frame(int fd, Frame *f, size_t f_size) {
  // frame variables to little-endian
  uint16_t _id = ntohs(f->id);

  LOG_MSG(LOG_INFO, "send_frame(id = %hd): start", _id);
  ssize_t bytes_sent = 0;
  ssize_t bytes_count;

  // bytes are possibly sent partially
  while (bytes_sent < (ssize_t)f_size) {
    // set socket for select and timeout
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(fd, &writefds);
    struct timeval timeout;
    timeout.tv_sec = SEND_TIMEOUT;
    timeout.tv_usec = 0;

    int retval = select(fd + 1, NULL, &writefds, NULL, &timeout);
    // socket not ready
    if (retval <= 0) {
      LOG_MSG(LOG_ERROR, "send_frame(id = %hd): socket not ready to send", _id);
      return -1;
    }

    // socket ready
    if (FD_ISSET(fd, &writefds)) {
      bytes_count = send(fd, f + bytes_sent, f_size - bytes_sent, 0);
      if (bytes_count <= 0) {
        LOG_MSG(LOG_ERROR, "send_frame(id = %hd): no bytes sent", _id);
        return -1;
      }
      bytes_sent += bytes_count;
      LOG_MSG(LOG_INFO, "send_frame(id = %hd): %ld bytes sent of %ld", _id,
              bytes_sent, f_size);
    }
  }

  // all bytes sent
  LOG_MSG(LOG_INFO, "send_frame(id = %hd): complete", _id);
  return 0;
}

// receive bytes and check if its a frame
int receive_frame(int fd, Frame *f, size_t f_size) {
  // frame variables to little endian
  uint16_t _id = ntohs(f->id);

  LOG_MSG(LOG_INFO, "receive_frame(id = %hd): start", _id);
  // set bytes
  memset(f, 0, f_size);

  // set socket and timeout for select
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  struct timeval timeout;
  timeout.tv_sec = RECV_TIMEOUT;
  timeout.tv_usec = 0;

  int retval = select(fd + 1, &readfds, NULL, NULL, &timeout);
  // socket not ready
  if (retval <= 0) {
    LOG_MSG(LOG_ERROR, "receive_frame(id = %hd): socket not ready to receive",
            _id);
    return -1;
  }

  // socket ready
  if (FD_ISSET(fd, &readfds)) {
    ssize_t bytes_count = recv(fd, f, f_size, 0);

    // no bytes received
    if (bytes_count <= 0) {
      LOG_MSG(LOG_ERROR, "receive_frame(id = %hd): no bytes received", _id);
      return -1;
    }

    // invalid frame
    if (check_valid_frame(f) != 0) {
      LOG_MSG(LOG_ERROR, "receive_frame(id = %hd): received invalid frame",
              _id);
      return -1;
    }

    // received valid frame
    LOG_MSG(LOG_INFO, "receive_frame(id = %hd): complete", _id);
    return 0;
  }

  // receipt failure
  return -1;
}
