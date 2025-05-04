// file:        server-client.h
// description: server/client functions

#ifndef CLIENT_SERVER_H
#define CLIENT_SERVER_H

#include <stdio.h>

int init_server(const char *protocol, const char *port_str);
int init_and_connect_client(const char *addr_str, const char *port_str);
void server_actions(int fd, FILE *input, FILE *output);
void client_md5_actions(int fd, const char *gas, size_t gas_size, FILE *output);
void client_xfer_actions(int fd, FILE *input, FILE *output);

#endif
