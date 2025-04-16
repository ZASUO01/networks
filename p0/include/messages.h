#ifndef MESSAGES_H
#define MESSAGES_H

#include <stdint.h>
#include <stdlib.h>

char *itr_operation(int fd, const char *id, uint32_t nonce);
int itv_operation(int fd, const char *sas);
char *gtr_operation(int fd, char **sas_list, int n);
int gtv_operation(int fd, char *gas);

#endif
