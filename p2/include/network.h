// file:        network.h
// description: definitions of network useful functions
#ifndef NETWORK_H
#define NETWORK_H

#include <stdlib.h>
#include <sys/socket.h>

#define PORT 55151

int parse_addr(const char *addr_str, struct sockaddr_storage *storage);
int create_and_bind_socket(const char *addr_str);
int send_packet(int fd, const char *dest_ip, const char *msg, size_t msg_size);
int receive_packet(int fd, int pipe_fd[2], char *msg, size_t msg_size);

#endif
