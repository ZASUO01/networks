#include "network.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

// try to convert the addr and port strings to a valid addr structure
int parse_addr(const char *addr_str, const char *port_str,
               struct sockaddr_storage *storage) {
  // can't parse null strings
  if (addr_str == NULL || port_str == NULL) {
    return -1;
  }

  // parse the port
  uint16_t port = (uint16_t)atoi(port_str);
  if (port == 0) {
    return -1;
  }

  port = htons(port);

  // try to parse IPv4
  struct in_addr in_addr4;
  if (inet_pton(AF_INET, addr_str, &in_addr4)) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_addr = in_addr4;
    addr4->sin_port = port;
    addr4->sin_family = AF_INET;
    return 0;
  }

  // try to parse IPv6
  struct in6_addr in_addr6;
  if (inet_pton(AF_INET6, addr_str, &in_addr6)) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    memcpy(&(addr6->sin6_addr), &in_addr6, sizeof(in_addr6));
    addr6->sin6_port = port;
    addr6->sin6_family = AF_INET6;
    return 0;
  }
  return -1;
}
