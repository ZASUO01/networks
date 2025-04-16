#include "parser.h"
#include "utils.h"

Params parse_args_md5(int argc, char **argv) {
  // min expected arguments
  if (argc < 4) {
    usage_md5(argv[0]);
  }

  // get params
  Params p;
  p.addr = argv[1];
  p.port = argv[2];
  p.gas = argv[3];

  return p;
}
