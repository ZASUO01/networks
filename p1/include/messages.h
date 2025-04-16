#ifndef MESSAGES_H
#define MESSAGES_H

#include "defs.h"
#include <stdlib.h>

extern MsgControl msg_control;

// function definitions
void print_frame(Frame *f);
void make_frame(Frame *f, uint16_t id, uint8_t flags, const char *data,
                size_t data_size);
int send_receive_auth(int fd, const char *data, size_t data_size);
int send_frame_receive_ack(int fd, Frame *req, size_t req_size, Frame *res,
                           size_t res_size);
int check_ack(Frame *f, size_t frame_size);
int receive_frame(int fd, Frame *f, size_t frame_size);
int send_ack(int fd);

#endif
