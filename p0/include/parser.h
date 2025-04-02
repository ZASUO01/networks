#ifndef PARSER_H
#define PARSER_H

// command line arguments
typedef struct {
  char *addr;
  char *port;
  char *cmd;
  char *id;
  char *gas;
  char *sas;
  char **sas_list;
  int nonce;
  int N;
} Params;

// parse the command line arguments
Params parse_args(int argc, char **argv);
void clean_params(Params *p);

#endif
