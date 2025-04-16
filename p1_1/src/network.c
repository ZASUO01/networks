#include <arpa/inet.h>
#include <netinet/in.h>
#include <network.h>
#include <stdlib.h>
#include <string.h>

// calculate the internet MD5 hash value from a given data
uint16_t get_md5_checksum(const void *frame, size_t frame_size) {
  // cast frame to bytes buffer
  const uint8_t *buf = (const uint8_t *)frame;

  // 4 byte return to avoid intermediate sums overflow
  uint32_t checksum = 0;

  // 2 byte to combine every byte pair
  uint16_t word;

  // iterate through the buffer with 2byte step
  for (size_t i = 0; i < frame_size; i += 2) {
    // combine the first and the second byte
    if (i + 1 < frame_size) {
      word = (buf[i] << 8) | buf[i + 1];
    }
    // combine the first byte and a 0 padding
    else {
      word = (buf[i] << 8) | 0;
    }

    checksum += word;

    // handle carry over 16bit
    while (checksum >> 16) {
      checksum = (checksum & 0xFFFF) + (checksum >> 16);
    }
  }

  // return 2byte 1 complement
  return (~checksum & 0xFFFF);
}

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
