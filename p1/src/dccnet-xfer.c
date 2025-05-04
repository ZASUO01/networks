#include "logger.h"
#include "msg-controller.h"
#include "parser.h"
#include "server-client.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // parse command line arguments
  Params p = parse_args_xfer(argc, argv);

  // set debug mode
  if (p.debug_mode > 0) {
    set_log_level(LOG_DEBUG);
  }

  // init global controller
  init_msg_controller(&msg_controller);
  printf("%s\n", p.input_file);
  // input file
  FILE *input_file = fopen(p.input_file, "r");
  if (input_file == NULL) {
    log_exit("input file failure");
  }

  // output file
  FILE *output_file = fopen(p.output_file, "w");
  if (output_file == NULL) {
    log_exit("output file failure");
  }

  // program socket
  int sock_fd;

  // server program
  if (p.server_side > 0) {
    sock_fd = init_server(p.ip_version, p.port);
    server_actions(sock_fd, input_file, output_file);
  }

  // client program
  else if (p.client_side > 0) {
    sock_fd = init_and_connect_client(p.addr, p.port);
    client_xfer_actions(sock_fd, input_file, output_file);

    // parameters dinamicaly allocated
    free(p.addr);
    free(p.port);
  }

  // destroy mutex and cond
  clean_msg_controller(&msg_controller);
  fclose(input_file);
  fclose(output_file);
  close(sock_fd);
  return 0;
}
