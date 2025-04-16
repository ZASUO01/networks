#include "parser.h"
#include "utils.h"

Params parse_args(int argc, char **argv) {
  // min expected arguments
  if (argc < 4) {
    usage(argv[0]);
  }

  // get params
  Params p;
  p.addr = argv[1];
  p.port = argv[2];
  p.gas = argv[3];

  return p;
}

// free memory allocated for params
void clean_params(Params *p) {}
