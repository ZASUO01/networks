#ifndef NETWORK_H
#define NETWORK_H

#include <sys/socket.h>

int parse_addr(const char *addr_str, const char *port_str,
               struct sockaddr_storage *storage);
char *send_receive_itr(int sock_fd, const char *id, int nonce);
char *send_receive_itv(int sock_fd, const char *sas);
char *send_receive_gtr(int sock_fd, char **sas_list, int n);
char *send_receive_gtv(int sock_fd, char *gas);
#endif
