#include "defs.h"
#include "messages.h"
#include "network.h"
#include "parser.h"
#include "utils.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

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
  timeout.tv_sec = 0;
  timeout.tv_usec = 700000;

  if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <
      0) {
    log_exit("Set socket timeout failure");
  }

  // connect to the server
  if (connect(sock_fd, server_addr, sizeof(storage)) != 0) {
    log_exit("Server connection failure");
  }

  // set network messages control values
  msg_control.current_id = 0;

  // grading 1
  size_t gas_size = strlen(p.gas);
  if (gas_size + 2 > MAX_DATA_BYTES) {
    log_exit("GAS size exceeds maximum size");
  }

  // full message from server
  char *msg = "";

  // request
  Frame request;
  make_frame(&request, 0, NO_FLAGS, p.gas, gas_size);
  size_t req_size = FRAME_HEADER_BYTES + gas_size + 1;

  // respose
  Frame response;
  size_t res_size = FRAME_HEADER_BYTES + MAX_DATA_BYTES;

  int max_attempts = MAX_ATTEMPTS;
  int total_attempts = 0;

  ssize_t bytes_count;
  while (total_attempts < max_attempts) {
    total_attempts++;

    bytes_count = send(sock_fd, &request, req_size, 0);
    if (bytes_count < (ssize_t)req_size) {
      log_exit("Send failure");
    }

    bytes_count = recv(sock_fd, &response, res_size, 0);
    if (bytes_count > 0) {
      if (htonl(response.SYNC1) != SYNC_BYTES ||
          htonl(response.SYNC2) != SYNC_BYTES) {
        log_exit("Invalid bytes");
      }

      if (response.flags == ACKNOWLEDGE_FLAG && response.id == request.id) {
        print_frame(&response);
        break;
      }
    }
  }

  memset(&response, 0, res_size);
  bytes_count = recv(sock_fd, &response, res_size, 0);
  print_frame(&response);

  // close socket
  close(sock_fd);

  return 0;
}
