#include "defs.h"

#include "messages.h"
#include "msg-control.h"
#include "network.h"
#include "operations.h"
#include "parser.h"
#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

// main function to perform this file task
int dccnet_md5(int fd, const char *gas, size_t gas_size, FILE *output) {
  LOG_MSG(LOG_INFO, "dccnet_md5(): init\n");

  // init message control
  init_msg_control(&msg_control);

  // request structure
  Frame req;
  make_frame(&req, msg_control.current_id, NO_FLAGS, gas, gas_size, 1);
  size_t req_size = FRAME_HEADER_BYTES + gas_size + END_CHAR_BYTE;

  // response structure
  Frame res;
  size_t res_size = FRAME_HEADER_BYTES + MAX_DATA_BYTES;

  // send request receive ack
  if (send_frame_receive_ack(fd, &req, req_size) != 0) {
    LOG_MSG(LOG_INFO, "dccnet_md5(): end with failure\n");
    return -1;
  }

  // full line buffer
  char full_msg[MAX_DATA_BYTES];
  full_msg[0] = '\0';

  // get lines until end receipt
  while (msg_control.recv_end == 0) {
    // try to get and handle the next response
    if (handle_responses(fd, &res, res_size) != 0) {
      LOG_MSG(LOG_INFO, "response not handled, retry...\n");
      continue;
    }

    // concatenate the received msg
    strcat(full_msg, msg_control.last_recv_data);

    // check line end
    size_t data_size = strlen(full_msg);
    if (full_msg[data_size - 1] == '\n') {
      // split msg in '\n'
      char *sub_msg = strtok(full_msg, "\n");

      // for each substring
      while (sub_msg != NULL) {
        // print line
        fprintf(output, "%s\n", sub_msg);

        // remove line break
        // full_msg[data_size - 1] = '\0';
        // data_size--;
        data_size = strlen(sub_msg);

        // get md5 hash
        char *md5_hash = get_md5_str(sub_msg);
        size_t hash_size = strlen(md5_hash);

        // make request
        make_frame(&req, msg_control.current_id, NO_FLAGS, md5_hash, hash_size,
                   1);
        req_size = FRAME_HEADER_BYTES + hash_size + END_CHAR_BYTE;

        // send the md5 hash and receive ack
        if (send_frame_receive_ack(fd, &req, req_size) != 0) {
          return -1;
        }

        // get next split
        sub_msg = strtok(NULL, "\n");
      }
      full_msg[0] = '\0';
    }
  }

  LOG_MSG(LOG_INFO, "dccnet_md5(): end with success\n");
  return 0;
}

int main(int argc, char **argv) {
  // parse arguments
  Params p = parse_args_md5(argc, argv);

  // paser server addr
  struct sockaddr_storage storage;
  if (parse_addr(p.addr, p.port, &storage) != 0) {
    log_exit("Addr parsing error");
  }
  struct sockaddr *server_addr = (struct sockaddr *)&storage;

  // create TCP socket
  int sock_fd = socket(storage.ss_family, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    log_exit("Socket creation failure");
  }

  // set socket timeout
  struct timeval timeout;
  timeout.tv_sec = 1;
  timeout.tv_usec = 0;

  if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <
      0) {
    close(sock_fd);
    log_exit("Set socket timeout failure");
  }

  // connect to the server
  if (connect(sock_fd, server_addr, sizeof(storage)) != 0) {
    close(sock_fd);
    log_exit("Server connection failure");
  }

  // set logger level
  if (p.debug_mode) {
    set_log_level(LOG_DEBUG);
  }

  // check if gas can be used
  size_t gas_size = strlen(p.gas);
  if (gas_size + 2 > MAX_DATA_BYTES) {
    close(sock_fd);
    log_exit("GAS size exceeds maximum size");
  }

  FILE *output = fopen("output/dccnet-md5.txt", "w");
  if (output == NULL) {
    close(sock_fd);
    log_exit("Failed to open output file");
  }

  // comunicate with server
  if (dccnet_md5(sock_fd, p.gas, gas_size, output) != 0) {
    close(sock_fd);
    log_exit("Failed to receive lines from server");
  }

  // close socket
  close(sock_fd);

  return 0;
}
