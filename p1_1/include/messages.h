#ifndef MESSAGES_H
#define MESSAGES_H

#include "defs.h"

void set_frame(Frame *f, const char *data, size_t data_size);
void to_big_endian(Frame *f);

void send_receive(int fd, void *request, int req_size, void *response,
                  int res_size);

#endif
