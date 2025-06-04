// file:        parser.c
// description: implementation of command line arguments parser
#include "parser.h"
#include "logger.h"
#include <stdlib.h>
#include <string.h>

// parse command line arguments and return them
Params parse_args(int argc, char **argv) {
  // min arguments expected
  if (argc < 3 || argc >= 6) {
    usage(argv[0]);
  }

  // params initialization
  Params p;
  p.addr_str = argv[1];
  p.period = atoi(argv[2]);
  p.startup_file_name = NULL;
  p.debug_mode = 0;

  // optional params
  if (argc >= 4) {
    p.startup_file_name = argv[3];

    if (argc == 5 && (strcmp(argv[4], "-d") == 0)) {
      p.debug_mode = 1;
    }
  }

  return p;
}
