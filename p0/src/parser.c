#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Params parse_args(int argc, char **argv) {
  // min expected arguments
  if (argc < 4) {
    usage(argv[0]);
  }

  // get params
  Params p;
  p.addr = argv[1];
  p.port = argv[2];
  p.cmd = argv[3];
  p.nonce = 0;
  p.N = 0;
  p.id = NULL;
  p.sas = NULL;
  p.gas = NULL;
  p.sas_list = NULL;

  // individual token request
  if (strcmp(p.cmd, "itr") == 0) {
    if (argc < 6) {
      log_exit("Bad itr usage");
    }

    p.id = argv[4];
    p.nonce = atoi(argv[5]);
  }

  // indivitual token validation
  else if (strcmp(p.cmd, "itv") == 0) {
    if (argc < 5) {
      log_exit("Bad itv usage");
    }

    p.sas = argv[4];
  }

  // group token request
  else if (strcmp(p.cmd, "gtr") == 0) {
    if (argc < 5) {
      log_exit("Bad gtr usage");
    }

    p.N = atoi(argv[4]);

    if (argc < 5 + p.N) {
      log_exit("Bad gtr usage");
    }

    p.sas_list = (char **)malloc(p.N * sizeof(char *));
    for (int i = 5; i < 5 + p.N; i++) {
      p.sas_list[i - 5] = strdup(argv[i]);
    }
  }

  // group token validation request
  else if (strcmp(p.cmd, "gtv") == 0) {
    if (argc < 5) {
      log_exit("Bad gtv usage");
    }

    p.gas = strdup(argv[4]);
  }

  // invalid case
  else {
    log_exit("Invalid command");
  }

  return p;
}

// free memory allocated for params
void clean_params(Params *p) {
  if (p->gas != NULL) {
    free(p->gas);
  }

  if (p->N > 0 && p->sas_list != NULL) {
    for (int i = 0; i < p->N; i++) {
      free(p->sas_list[i]);
    }
    free(p->sas_list);
  }
}
