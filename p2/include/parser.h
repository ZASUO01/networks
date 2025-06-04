// file:        parser.h
// description: definitions for the command line arguments parser
#ifndef PARSER_H
#define PARSER_H

// command line arguments
typedef struct {
  char *addr_str;
  char *startup_file_name;
  int period;
  int debug_mode;
} Params;

// parse the command line arguments
Params parse_args(int argc, char **argv);

#endif
