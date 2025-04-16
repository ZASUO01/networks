#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <sys/socket.h>

uint16_t get_md5_checksum(const void *frame, size_t frame_size);
int parse_addr(const char *addr_str, const char *port_str,
               struct sockaddr_storage *storage);
#endif
