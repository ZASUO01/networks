#include "defs.h"
#include "logger.h"
#include "msg-controller.h"
#include "parser.h"
#include "server-client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // parse command line arguments
  Params p = parse_args_md5(argc, argv);

  // enable logger
  if (p.debug_mode > 0) {
    set_log_level(LOG_DEBUG);
  }

  // output file
  FILE *output_file;
  if (p.output_file == NULL) {
    output_file = stderr;
  } else {
    output_file = fopen(p.output_file, "w");
    if (output_file == NULL) {
      log_exit("output file error");
    }
  }

  // init messages controller
  init_msg_controller(&msg_controller);

  // check gas
  size_t gas_size = strlen(p.gas);
  if (gas_size + 2 > MAX_DATA_BYTES) {
    log_exit("invalid gas size");
  }

  // dccnet md5
  int sock_fd = init_and_connect_client(p.addr, p.port);
  client_md5_actions(sock_fd, p.gas, gas_size, output_file);

  // finish procedures
  clean_msg_controller(&msg_controller);
  close(sock_fd);
  fclose(output_file);
  free(p.addr);
  free(p.port);
  return 0;
}
