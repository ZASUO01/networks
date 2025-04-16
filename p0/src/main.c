#include "messages.h"
#include "network.h"
#include "parser.h"
#include "utils.h"
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
  timeout.tv_sec = 2;
  timeout.tv_usec = 0;

  if (setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) <
      0) {
    log_exit("Set socket timeout failure");
  }

  // connect to the server
  if (connect(sock_fd, server_addr, sizeof(storage)) != 0) {
    log_exit("Server connection failure");
  }

  // set log level
  set_log_level(LOG_DEBUG);

  // performm operations
  char *response_str = NULL;
  int response_int = -1;

  // individual token request
  if (strcmp(p.cmd, "itr") == 0) {
    response_str = itr_operation(sock_fd, p.id, p.nonce);
  }

  // individual token validation
  else if (strcmp(p.cmd, "itv") == 0) {
    response_int = itv_operation(sock_fd, p.sas);
  }

  // group token request
  else if (strcmp(p.cmd, "gtr") == 0) {
    response_str = gtr_operation(sock_fd, p.sas_list, p.N);
  }

  // group token validation
  else {
    response_int = gtv_operation(sock_fd, p.gas);
  }

  // print the response
  if (response_str) {
    printf("%s\n", response_str);
    free(response_str);
  } else if (response_int >= 0) {
    printf("%d\n", response_int);
  } else {
    log_exit("Internal server error");
  }

  // free all dinamically alocated memory
  clean_params(&p);

  // close socket
  close(sock_fd);

  return 0;
}
