#include "network.h"
#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // parse command line arguments
  Params p = parse_args(argc, argv);

  // get the proper server addr
  struct sockaddr_storage storage;
  if (parse_addr(p.addr, p.port, &storage) != 0) {
    log_exit("Addr parsing failure");
  }
  struct sockaddr *server_addr = (struct sockaddr *)&storage;

  // create udp socket
  int sock_fd = socket(storage.ss_family, SOCK_DGRAM, 0);
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

  // performm operations
  char *response;

  // individual token request
  if (strcmp(p.cmd, "itr") == 0) {
    response = send_receive_itr(sock_fd, p.id, p.nonce);
  }

  // individual token validation
  else if (strcmp(p.cmd, "itv") == 0) {
    response = send_receive_itv(sock_fd, p.sas);
  }

  // group token request
  else if (strcmp(p.cmd, "gtr") == 0) {
    response = send_receive_gtr(sock_fd, p.sas_list, p.N);
  }

  // group token validation
  else {
    response = send_receive_gtv(sock_fd, p.gas);
  }

  // print the response
  if (response) {
    printf("%s\n", response);
    free(response);
  } else {
    log_exit("No response from server");
  }

  // free all dinamically alocated memory
  clean_params(&p);

  // close socket
  close(sock_fd);

  return 0;
}
