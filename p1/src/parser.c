#include "parser.h"
#include "logger.h"
#include <string.h>

// parse the format <IP>:<PORT> into program params
static void parse_ip_and_port(Params *p, const char *str, const char *program) {
  char buff[128];

  // copy the argument <IP>:<PORT> to a buffer
  strncpy(buff, str, sizeof(buff));
  buff[sizeof(buff) - 1] = '\0';

  // look for the last ':' char
  char *last_colon = strrchr(buff, ':');
  if (!last_colon) {
    usage_xfer(program);
  }

  // split the buffer in the found char
  *last_colon = '\0';
  p->addr = strdup(buff);
  p->port = strdup(last_colon + 1);
}

// paser command line arguments for md5 program
Params parse_args_md5(int argc, char **argv) {
  // check min arguments number
  if (argc < 3) {
    usage_md5(argv[0]);
  }

  // init program variables
  Params p;
  p.addr = NULL;
  p.port = NULL;
  p.gas = NULL;
  p.output_file = NULL;
  p.debug_mode = 0;

  // ip and port argument
  parse_ip_and_port(&p, argv[1], argv[0]);

  // next argument
  p.gas = argv[2];

  // optional extra arguments
  if (argc >= 4) {
    p.output_file = argv[3];
    if (argc == 5 && strcmp(argv[4], "-d") == 0) {
      p.debug_mode = 1;
    }
  }

  return p;
}

// parse command line arguments for xfer program
Params parse_args_xfer(int argc, char **argv) {
  // check min arguments number
  if (argc < 5) {
    usage_xfer(argv[0]);
  }

  // program params
  Params p;
  p.server_side = 0;
  p.client_side = 0;
  p.addr = NULL;
  p.port = NULL;
  p.input_file = NULL;
  p.output_file = NULL;
  p.debug_mode = 0;
  p.ip_version = "v4";

  // server mode
  if (strcmp(argv[1], "-s") == 0) {
    // the server expects the port only
    p.server_side = 1;
    p.port = argv[2];
  }

  // client mode
  else if (strcmp(argv[1], "-c") == 0) {
    // the client expects the server addr and port
    p.client_side = 1;

    // parse addr and port
    char *input = argv[2];
    char buff[128];

    strncpy(buff, input, sizeof(buff));
    buff[sizeof(buff) - 1] = '\0';

    char *last_colon = strrchr(buff, ':');
    if (!last_colon) {
      usage_xfer(argv[0]);
    }

    *last_colon = '\0';
    p.addr = strdup(buff);
    p.port = strdup(last_colon + 1);
  }

  // invalid mode
  else {
    usage_xfer(argv[0]);
  }

  // set in/out files
  p.input_file = argv[3];
  p.output_file = argv[4];

  // optional param
  if (argc >= 6 && (strcmp(argv[5], "v4") == 0 || strcmp(argv[5], "v6") == 0)) {
    p.ip_version = argv[5];
    if (argc == 7 && strcmp(argv[6], "-d") == 0) {
      p.debug_mode = 1;
    }
  }

  return p;
}
