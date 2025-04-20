// file:        parser.h
// description: definitions for the command line arguments parser
#ifndef PARSER_H
#define PARSER_H

// command line arguments
typedef struct {
  int debug_mode;
  char *addr;
  char *port;
  char *gas;
} Params;

// parse the command line arguments
Params parse_args_md5(int argc, char **argv);
Params parse_args_xfer(int argc, char **argv);

#endif
