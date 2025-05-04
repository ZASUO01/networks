// file:        operations.h
// description: definitions for the main operations performed in the programs
#ifndef OPERATIONS_H
#define OPERATIONS_H

#include <stdio.h>

// thread arguments
typedef struct {
  int fd;
  FILE *input;
  FILE *output;
  char *gas;
  size_t gas_size;
} ThreadArgs;

// thread functions
void *send_md5_thread(void *arg);
void *send_xfer_thread(void *arg);
void *receive_thread(void *arg);
void *ack_xfer_thread(void *arg);
void *print_thread(void *arg);

#endif
