#include "operations.h"
#include "defs.h"
#include "messages.h"
#include "msg-control.h"
#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

// send reset message
static int send_reset(int fd) {
  LOG_MSG(LOG_INFO, "send_reset(): init\n");

  // reset frame
  Frame f;
  make_frame(&f, RESET_ID, RESET_FLAG, NULL, 0, 0);

  // try to send the frame
  if (send_frame(fd, &f, FRAME_HEADER_BYTES) != 0) {
    LOG_MSG(LOG_ERROR, "send_reset(): end with no ack sent\n");
    return -1;
  }

  LOG_MSG(LOG_INFO, "send_reset(): end with reset sent\n");
  return 0;
}

// send ack message
static int send_ack(int fd, uint16_t id) {
  LOG_MSG(LOG_INFO, "send_ack(): init\n");

  // ack frame
  Frame f;
  make_frame(&f, id, ACKNOWLEDGE_FLAG, NULL, 0, 0);

  // try to send the frame
  if (send_frame(fd, &f, FRAME_HEADER_BYTES) != 0) {
    LOG_MSG(LOG_ERROR, "send_ack(): end with no ack sent\n");
    return -1;
  }

  LOG_MSG(LOG_INFO, "send_ack(): end with ack sent\n");
  return 0;
}

// send a frame and wait for an ack response
int send_frame_receive_ack(int fd, Frame *req, size_t req_size) {
  LOG_MSG(LOG_INFO, "send_frame_receive_ack(): init\n");

  // respponse structure
  Frame res;
  size_t res_size = FRAME_HEADER_BYTES + MAX_DATA_BYTES;

  // attempts to be performed
  int total_attempts = 0;
  set_waiting_ack(&msg_control);

  while (total_attempts < MAX_ATTEMPTS) {
    total_attempts++;
    LOG_MSG(LOG_INFO, "attempt %d\n", total_attempts);

    // send request
    if (send_frame(fd, req, req_size) != 0) {
      LOG_MSG(LOG_WARNING, "message send failure, retry...\n");
      continue;
    }

    // handle possible responses
    if (handle_responses(fd, &res, res_size) != 0) {
      LOG_MSG(LOG_WARNING, "handle response failure, retry...\n");
      continue;
    }

    // got ack response
    if (res.flags == ACKNOWLEDGE_FLAG &&
        ntohs(res.id) == msg_control.current_id) {
      LOG_MSG(LOG_INFO, "send_frame_receive_ack(): end with ack\n");
      set_waiting_ack(&msg_control);
      set_next_id(&msg_control);
      return 0;
    }

    LOG_MSG(LOG_WARNING, "no ack received, retry...\n");
  }

  // failled all attempts
  // send reset
  if (send_reset(fd) != 0) {
    LOG_MSG(LOG_ERROR,
            "send_frame_receive_ack(): end without ack and reset not sent\n");
  } else {
    LOG_MSG(LOG_ERROR,
            "send_frame_receive_ack(): end without ack and reset sent\n");
  }
  return -1;
}

//
int handle_responses(int fd, Frame *res, size_t res_size) {
  LOG_MSG(LOG_INFO, "handle_responses(): init\n");

  // try to receive the frame
  if (receive_frame(fd, res, res_size) != 0) {
    return -1;
  }

  // received reset frame, abort
  if (res->flags == RESET_FLAG) {
    LOG_MSG(LOG_ERROR, "received reset frame: %s", res->data);
    close(fd);
    log_exit("Exit with reset signal");
  }

  // receive data frame
  if (res->flags == NO_FLAGS || res->flags == END_FLAG) {
    LOG_MSG(LOG_INFO, "received data frame\n");

    // end flag
    if (res->flags == END_FLAG) {
      LOG_MSG(LOG_INFO, "received end frame\n");
      set_recv_end(&msg_control);
    }

    // this data has been received before
    if (ntohs(res->id) == msg_control.last_recv_id) {
      LOG_MSG(LOG_WARNING, "data already received, send ack again\n");
      send_ack(fd, msg_control.last_recv_id);
      return -1;
    }

    // received data while waiting for ack
    // the last ack sent is possibly lost
    if (msg_control.waiting_ack == 1) {
      LOG_MSG(LOG_INFO, "%s\n", res->data);
      LOG_MSG(LOG_WARNING, "retransmit last ack\n");
      send_ack(fd, msg_control.last_recv_id);
      return -1;
    }

    // received data, send ack
    if (send_ack(fd, ntohs(res->id)) != 0) {
      LOG_MSG(LOG_ERROR, "ack not sent\n");
      return -1;
    }

    size_t data_size = strlen(res->data);
    if (res->data[data_size - 1] == '\n') {
      LOG_MSG(LOG_INFO, "data: %s", res->data);
    } else {
      LOG_MSG(LOG_INFO, "data: %s\n", res->data);
    }

    res->data[data_size] = '\0';

    set_last_recv_id(&msg_control, ntohs(res->id));
    set_last_data(&msg_control, res->data, data_size);

    return 0;
  }

  // got ack while not waiting for ack
  if (res->flags == ACKNOWLEDGE_FLAG && msg_control.waiting_ack == 0) {
    // duplicated ack
    if (ntohs(res->id) == msg_control.last_id) {
      LOG_MSG(LOG_WARNING, "got duplicated ack\n");
      return -1;
    }

    LOG_MSG(LOG_WARNING, "got ack while not waiting for ack\n");
    return -1;
  }

  LOG_MSG(LOG_INFO, "handle_responses(): end with success\n");
  return 0;
}
