#ifndef PARSER_H
#define PARSER_H

// command line arguments
typedef struct {
  char *addr;
  char *port;
  char *gas;
} Params;

// parse the command line arguments
Params parse_args(int argc, char **argv);
void clean_params(Params *p);

#endif
