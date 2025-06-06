// file:        network.h
// description: definitions net util functions
#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <sys/socket.h>

int init_server_addr(const char *protocol, const char *port_str,
                     struct sockaddr_storage *storage);
int parse_addr(const char *addr_str, const char *port_str,
               struct sockaddr_storage *storage);
uint16_t get_checksum(void *frame, size_t frame_size);
char *get_md5_str(const char *data);

#endif
