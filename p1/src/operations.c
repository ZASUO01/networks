#include "operations.h"
#include "defs.h"
#include "logger.h"
#include "messages.h"
#include "msg-controller.h"
#include "network.h"
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// send an ack frame
static int send_ack(int fd, uint16_t id) {
  // ack frame
  Frame ack;
  make_frame(&ack, id, ACKNOWLEDGE_FLAG, NULL, 0, 0);

  // try to send the ack
  return send_frame(fd, &ack, FRAME_HEADER_BYTES);
}

// send an end frame
static int send_end(int fd, uint16_t id) {
  // end frame
  Frame end;
  make_frame(&end, id, END_FLAG, NULL, 0, 0);

  // try to send the end
  return send_frame(fd, &end, FRAME_HEADER_BYTES);
}

// send a frame and wait for the ack in a limited timeout
// if the timeout is reached, the frame is retransmited
static int send_data_wait_ack(int fd, char *data, size_t data_size,
                              int end_char) {
  // get current id for reference
  pthread_mutex_lock(&msg_controller.mc_mutex);
  uint16_t id = msg_controller.current_id;
  pthread_mutex_unlock(&msg_controller.mc_mutex);

  LOG_MSG(LOG_INFO, "send_data_wait_ack(id = %hd): start", id);

  // Data Frame
  Frame req;
  make_frame(&req, id, NO_FLAGS, data, data_size, end_char);
  size_t req_size = FRAME_HEADER_BYTES + data_size;
  if (end_char > 0) {
    req_size += END_CHAR_BYTE;
    req.data[data_size + END_CHAR_BYTE] = '\0';
  }

  // attempts to send
  int total_attempts = 0;
  while (total_attempts < MAX_ATTEMPTS) {
    total_attempts++;
    LOG_MSG(LOG_INFO, "send_data_wait_ack(id = %hd): attempt %d", id,
            total_attempts);

    // try to send, and retry if failed
    if (send_frame(fd, &req, req_size) != 0) {
      LOG_MSG(LOG_WARNING,
              "send_data_wait_ack(id = %hd): failed to send, retry...", id);
      continue;
    }

    LOG_MSG(LOG_INFO,
            "send_data_wait_ack(id = %hd): data sent, waiting for ack\n%s", id,
            req.data);

    // wait for ack, before sending another data or retransmiting
    pthread_mutex_lock(&msg_controller.mc_mutex);
    msg_controller.waiting_ack = 1;
    msg_controller.last_sent_id = id;

    // set wait timeout
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    ts.tv_sec += 1;
    ts.tv_nsec += 0;
    int ret = pthread_cond_timedwait(&msg_controller.mc_ack_cond,
                                     &msg_controller.mc_mutex, &ts);

    // timeout reached
    if (ret == ETIMEDOUT) {
      LOG_MSG(LOG_WARNING,
              "send_data_wait_ack(id = %hd): wait ack timeout, retransmit...",
              id);
      pthread_mutex_unlock(&msg_controller.mc_mutex);
      continue;
    }

    // received ack, no need to retransmit
    if (msg_controller.waiting_ack == 0) {
      LOG_MSG(LOG_INFO,
              "send_data_wait_ack(id = %hd): complete due ack received", id);
      pthread_mutex_unlock(&msg_controller.mc_mutex);
      return 0;
    }

    pthread_mutex_unlock(&msg_controller.mc_mutex);
  }

  // all attempts failed
  LOG_MSG(LOG_ERROR, "send_data_wait_ack(id = %hd): complete with failure", id);
  return -1;
}

// receive frames from network until end frame received and sent
void *receive_thread(void *arg) {
  LOG_MSG(LOG_INFO, "receive_thread(): start");

  // thread arguments
  ThreadArgs *tr = (ThreadArgs *)arg;

  // receipt frame
  Frame rec;
  size_t rec_size = FRAME_HEADER_BYTES + MAX_DATA_BYTES;

  // attempts to be performed
  int total_attempts = 0;
  while (total_attempts < MAX_ATTEMPTS) {
    total_attempts++;

    // stop condition, received and and sent end
    pthread_mutex_lock(&msg_controller.mc_mutex);
    if (msg_controller.received_end > 0 && msg_controller.sent_end > 0) {
      pthread_mutex_unlock(&msg_controller.mc_mutex);
      LOG_MSG(LOG_INFO, "receive_thread(): complete due end flag");
      break;
    }
    pthread_mutex_unlock(&msg_controller.mc_mutex);

    LOG_MSG(LOG_INFO, "receive_thread(): attempt %d", total_attempts);

    //  try to receive the frame and retry if failed
    if (receive_frame(tr->fd, &rec, rec_size) != 0) {
      LOG_MSG(LOG_WARNING, "receive_thread(): receipt failed, retry..");
      continue;
    }

    LOG_MSG(LOG_INFO, "receive_thread(): received flag %x", rec.flags);
    total_attempts = 0;

    uint16_t rec_id = ntohs(rec.id);
    pthread_mutex_lock(&msg_controller.mc_mutex);

    // received reset flag, abort program
    if (rec.flags == RESET_FLAG) {
      close(tr->fd);
      log_exit("received reset");
    }

    // received ack frame
    if (rec.flags == ACKNOWLEDGE_FLAG) {
      // received expected ack while waiting for ack
      if (msg_controller.waiting_ack > 0 &&
          rec_id == msg_controller.last_sent_id) {
        msg_controller.waiting_ack = 0;
        msg_controller.current_id = 1 - msg_controller.current_id;
        pthread_cond_signal(&msg_controller.mc_ack_cond);
      }

      LOG_MSG(LOG_INFO,
              "receive_thread(): received ack id %hd while current id %hd",
              rec_id, msg_controller.last_sent_id);
    }

    // received data or end
    else if (rec.flags == NO_FLAGS || rec.flags == END_FLAG) {
      // new frame
      if (rec_id != msg_controller.last_received_id) {
        msg_controller.last_received_id = rec_id;
        msg_controller.new_data_available = 1;

        // received end
        if (rec.flags == END_FLAG) {
          LOG_MSG(LOG_INFO, "receive_thread(): received new end frame id %hd",
                  rec_id);
          msg_controller.received_end = 1;
        } else {
          LOG_MSG(LOG_INFO, "receive_thread(): received new data frame id %hd",
                  rec_id);
          msg_controller.last_size_received = ntohs(rec.lenght);
          set_last_received_data(&msg_controller, rec.data,
                                 msg_controller.last_size_received);
        }

        pthread_cond_signal(&msg_controller.mc_data_cond);
      } else {
        LOG_MSG(LOG_INFO, "receive_thread(): received duplicated frame id %hd",
                rec_id);
      }

      // send ack even for duplicated data
      LOG_MSG(LOG_INFO, "receive_thread(): need to send ack id %hd", rec_id);
      if (send_ack(tr->fd, rec_id) != 0) {
        LOG_MSG(LOG_ERROR, "receive_thread(): failed to send ack id %hd",
                rec_id);
      }
      LOG_MSG(LOG_INFO, "receive_thread(): akc id %hd sent", rec_id);
    }

    pthread_mutex_unlock(&msg_controller.mc_mutex);
  }

  LOG_MSG(LOG_INFO, "receive_thread(): complete");
  return NULL;
}

// send thread used in dccnet-md5 program
void *send_md5_thread(void *arg) {
  LOG_MSG(LOG_INFO, "send_md5_thread(): start");

  // thread arguments
  ThreadArgs *tr = (ThreadArgs *)arg;

  // init authentication
  if (send_data_wait_ack(tr->fd, tr->gas, tr->gas_size, 1) != 0) {
    LOG_MSG(LOG_ERROR,
            "send_md5_thread(): complete with no ack after gas sent");
    return NULL;
  }

  // send md5 hash for the received data
  char full_msg[MAX_DATA_BYTES];
  full_msg[0] = '\0';

  LOG_MSG(LOG_INFO,
          "send_md5_thread(): prepare to send md5 hash of the received data");
  while (1) {
    // wait for new data or end
    pthread_mutex_lock(&msg_controller.mc_mutex);
    if (msg_controller.new_data_available == 0) {
      LOG_MSG(LOG_INFO, "send_md5_thread(): waiting for new data");
      pthread_cond_wait(&msg_controller.mc_data_cond, &msg_controller.mc_mutex);
    }

    // stop condition, received end
    if (msg_controller.received_end > 0) {
      LOG_MSG(LOG_INFO,
              "send_md5_thread(): no need to send md5 hash due end received");
      pthread_mutex_unlock(&msg_controller.mc_mutex);
      break;
    }

    // concatenate received data
    msg_controller.last_received_data[msg_controller.last_size_received] = '\0';
    strcat(full_msg, msg_controller.last_received_data);
    msg_controller.new_data_available = 0;
    LOG_MSG(LOG_INFO, "send_md5_thread(): new data to hash: %s",
            msg_controller.last_received_data);
    pthread_mutex_unlock(&msg_controller.mc_mutex);

    size_t data_size = strlen(full_msg);
    if (full_msg[data_size - 1] == '\n') {
      // split msg in '\n'
      char *sub_msg = strtok(full_msg, "\n");

      while (sub_msg != NULL) {
        // print line
        fprintf(tr->output, "%s\n", sub_msg);

        // hash data
        char *md5_hash = get_md5_str(sub_msg);
        size_t md5_hash_size = strlen(md5_hash);

        if (send_data_wait_ack(tr->fd, md5_hash, md5_hash_size, 1) != 0) {
          LOG_MSG(LOG_ERROR, "send_md5_thread(): failed to send hash");
          break;
        }
        LOG_MSG(LOG_INFO, "send_md5_thread(): hash sent %s\n", md5_hash);

        sub_msg = strtok(NULL, "\n");
      }
      full_msg[0] = '\0';
    }
  }

  LOG_MSG(LOG_INFO, "send_md5_thread(): complete");
  return NULL;
}

// send thread used by xfer program
void *send_xfer_thread(void *arg) {
  LOG_MSG(LOG_INFO, "send_xfer_thread(): start");

  // thread arguments
  ThreadArgs *tr = (ThreadArgs *)arg;

  // buffer to be read
  char line[MAX_DATA_BYTES];
  memset(line, 0, MAX_DATA_BYTES);

  // read bytes until end
  ssize_t bytes_read;
  while ((bytes_read = fread(line, 1, MAX_DATA_BYTES, tr->input)) > 0) {
    LOG_MSG(LOG_INFO, "send_xfer_thread(): sending data of %ld bytes read",
            bytes_read);

    // send bytes and wait for ack
    if (send_data_wait_ack(tr->fd, line, bytes_read, 0) != 0) {
      LOG_MSG(LOG_ERROR,
              "send_xfer_thread(): failed to send file linea and get ack");
      return NULL;
    }
  }

  LOG_MSG(LOG_INFO, "send_xfer_thread(): file transfered, need to send end");

  // send end after complete file transfer
  pthread_mutex_lock(&msg_controller.mc_mutex);
  if (send_end(tr->fd, msg_controller.current_id) != 0) {
    LOG_MSG(LOG_ERROR, "send_xfer_thread(): failed to send end frame");
    pthread_mutex_unlock(&msg_controller.mc_mutex);
    return NULL;
  }

  msg_controller.sent_end = 1;
  pthread_mutex_unlock(&msg_controller.mc_mutex);

  LOG_MSG(LOG_INFO, "send_xfer_thread(): send end");

  LOG_MSG(LOG_INFO, "send_xfer_thread(): complete");
  return NULL;
}

// thread to print bytes to output file
void *print_thread(void *arg) {
  LOG_MSG(LOG_INFO, "print_thread(): start");

  // total bytes to print
  size_t total_bytes = 0;

  // thread arguments
  ThreadArgs *tr = (ThreadArgs *)arg;

  while (1) {
    // wait for new data or end
    pthread_mutex_lock(&msg_controller.mc_mutex);
    if (msg_controller.new_data_available == 0) {
      pthread_cond_wait(&msg_controller.mc_data_cond, &msg_controller.mc_mutex);
    }

    // stop condition, received end
    if (msg_controller.received_end > 0) {
      LOG_MSG(LOG_INFO, "print_thread(): end received");
      pthread_mutex_unlock(&msg_controller.mc_mutex);
      break;
    }

    // write bytes to file
    ssize_t bytes_written =
        fwrite(msg_controller.last_received_data, 1,
               msg_controller.last_size_received, tr->output);
    if (bytes_written <= 0) {
      log_exit("failed to write to output");
    }
    msg_controller.new_data_available = 0;

    // print progress
    total_bytes += msg_controller.last_size_received;
    printf("%ld bytes received\n", total_bytes);

    pthread_mutex_unlock(&msg_controller.mc_mutex);
  }
  LOG_MSG(LOG_INFO, "print_thread(): complete");
  return NULL;
}
