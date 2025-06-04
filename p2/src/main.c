#include "logger.h"
#include "network.h"
#include "operations.h"
#include "parser.h"
#include "router.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv) {
  // parse command line arguments
  Params p = parse_args(argc, argv);

  // set debug mode
  FILE *log_file = NULL;
  if (p.debug_mode > 0) {
    char *log_file_name = get_log_file_name(p.addr_str);
    log_file = fopen(log_file_name, "w");
    if (log_file == NULL) {
      log_exit("log file failure");
    }
    set_log_file(log_file);
    set_log_level(LOG_INFO);
    free(log_file_name);
  }

  // get udp socket
  int sock_fd = create_and_bind_socket(p.addr_str);
  LOG_MSG(LOG_INFO, "main(): UPD socket created and bounded with ip %s",
          p.addr_str);

  // initialize router
  init_router(&router, sock_fd, p.addr_str, p.period);
  LOG_MSG(LOG_INFO, "main(): router initialized");

  // read from startup file
  FILE *startup_file = NULL;
  if (p.startup_file_name != NULL) {
    startup_file = fopen(p.startup_file_name, "r");
    if (startup_file == NULL) {
      close(sock_fd);
      log_exit("startup file failure");
    }

    startup_router(&router, startup_file);
    LOG_MSG(LOG_INFO, "main(): router configured with startup file");
  }

  // program operations executed until quit command
  execute_operations(p.period);
  LOG_MSG(LOG_INFO, "main(): operations finished");

  // clean before exit
  if (log_file != NULL) {
    fclose(log_file);
  }

  if (startup_file != NULL) {
    fclose(startup_file);
  }

  pthread_mutex_destroy(&router.router_mutex);
  pthread_cond_destroy(&router.router_update_cond);
  close(sock_fd);
  return 0;
}
