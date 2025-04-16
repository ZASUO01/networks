#include "defs.h"
#include "messages.h"
#include "network.h"
#include "parser.h"
#include "utils.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // parse arguments
  Params p = parse_args(argc, argv);

  // parse server addr
  struct sockaddr_storage storage;
  if (parse_addr(p.addr, p.port, &storage) != 0) {
    log_exit("Addr parsing failure");
  }
  struct sockaddr *server_addr = (struct sockaddr *)&storage;

  // create TCP socket
  int sock_fd = socket(storage.ss_family, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    log_exit("Socket creation failure");
  }

  // set socket timeout
  struct timeval timeout;
  timeout.tv_sec = 3;
  timeout.tv_usec = 0;

  if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <
      0) {
    log_exit("Set socket timeout failure");
  }

  // connect to the server
  if (connect(sock_fd, server_addr, sizeof(storage)) != 0) {
    log_exit("Server connection failure");
  }

  // send receive
  Frame frame;
  ssize_t gas_size = strlen(p.gas);
  set_frame(&frame, p.gas, gas_size);
  to_big_endian(&frame);

  printf("%x %x\n", frame.SYNC1, frame.SYNC2);
  printf("%d\n", frame.checksum);
  printf("%d\n", ntohs(frame.lenght));
  printf("%d\n", ntohs(frame.id));
  printf("%x\n", frame.flags);
  printf("%s", frame.data);

  Frame response;
  memset(&response, 0, sizeof(Frame));

  char buf[4096];
  int attempts = 0;
  int max_attempts = 4;
  while (attempts < max_attempts) {
    ssize_t bytes_count = send(sock_fd, &frame, FRAME_BYTES + gas_size + 1, 0);
    if (bytes_count != FRAME_BYTES + gas_size + 1) {
      log_exit("Send failure");
    }

    bytes_count = recv(sock_fd, &response, FRAME_BYTES + MAX_DATA_BYTES, 0);

    if (bytes_count < 0) {
      printf("failure\n");
    } else {
      printf("Success %ld\n", bytes_count);
      response.data[bytes_count - FRAME_BYTES] = '\0';
      strncpy(buf, (char *)response.data, bytes_count - FRAME_BYTES + 1);
      break;
    }
  }

  printf("%lu\n", ntohl(response.SYNC1));

  printf("%x %x\n", response.SYNC1, response.SYNC2);
  printf("%d\n", response.checksum);
  printf("%d\n", ntohs(response.lenght));
  printf("%d\n", ntohs(response.id));
  printf("%x\n", response.flags);
  printf("%s\n", buf);

  // close socket
  close(sock_fd);

  // clean params
  clean_params(&p);

  return 0;
}
