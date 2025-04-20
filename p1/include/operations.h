// file:        operations.h
// description: definitions for the main operations performed in the programs
#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "defs.h"
#include <stdio.h>
#include <stdlib.h>

int send_frame_receive_ack(int fd, Frame *req, size_t req_size);
int handle_responses(int fd, Frame *res, size_t res_size);

#endif
