#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// print the correct program usage and finish the program
void usage(const char *program) {
  printf("usage: %s <server IP> <server port> <command>\n", program);
  exit(EXIT_FAILURE);
}

// finish the program with an error message
void log_exit(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}
