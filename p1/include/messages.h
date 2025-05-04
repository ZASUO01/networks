// file:        messages.h
// description: definitions for the messages operations
#ifndef MESSAGES_H
#define MESSAGES_H

#include "defs.h"
#include <stdint.h>
#include <stdlib.h>

// function definitions
void make_frame(Frame *f, uint16_t id, uint8_t flags, const char *data,
                size_t data_size, int end_char);
int send_frame(int fd, Frame *f, size_t f_size);
int receive_frame(int fd, Frame *f, size_t f_size);

#endif
