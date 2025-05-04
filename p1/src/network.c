#include "network.h"
#include "logger.h"
#include <arpa/inet.h>
#include <openssl/evp.h>
#include <stdlib.h>
#include <string.h>

// init a server address with port
// protocol can be v6 or v4
int init_server_addr(const char *protocol, const char *port_str,
                     struct sockaddr_storage *storage) {
  // can't parse null strings
  if (protocol == NULL || port_str == NULL) {
    return -1;
  }

  // parse the port
  uint16_t port = (uint16_t)atoi(port_str);
  if (port == 0) {
    return -1;
  }
  port = htons(port);

  // get the addr for the given protocol
  if (strcmp(protocol, "v4") == 0) {
    struct sockaddr_in *addr4 = (struct sockaddr_in *)storage;
    addr4->sin_addr.s_addr = INADDR_ANY;
    addr4->sin_port = port;
    addr4->sin_family = AF_INET;
    return 0;
  } else if (strcmp(protocol, "v6") == 0) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    addr6->sin6_addr = in6addr_any;
    addr6->sin6_port = port;
    addr6->sin6_family = AF_INET6;
    return 0;
  }

  return -1;
}

// try to convert the addr and port strings to a valid addr structure
int parse_addr(const char *addr_str, const char *port_str,
               struct sockaddr_storage *storage) {
  LOG_MSG(LOG_INFO, "parse_addr(): start");

  // can't parse null strings
  if (addr_str == NULL || port_str == NULL) {
    LOG_MSG(LOG_ERROR, "parse_addr(): empty addr or port");
    return -1;
  }

  // parse the port
  uint16_t port = (uint16_t)atoi(port_str);
  if (port == 0) {
    LOG_MSG(LOG_ERROR, "parse_addr(): failed to parse port");
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

    LOG_MSG(LOG_INFO, "parse_addr(): complete with ipv4");
    return 0;
  }

  // try to parse IPv6
  struct in6_addr in_addr6;
  if (inet_pton(AF_INET6, addr_str, &in_addr6)) {
    struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)storage;
    memcpy(&(addr6->sin6_addr), &in_addr6, sizeof(in_addr6));
    addr6->sin6_port = port;
    addr6->sin6_family = AF_INET6;

    LOG_MSG(LOG_INFO, "parse_addr(): complete with ipv6");
    return 0;
  }

  LOG_MSG(LOG_ERROR, "parse_addr(): failed");
  return -1;
}

// internet checksum algorithm
uint16_t get_checksum(void *frame, size_t frame_size) {
  // cast frame as bytes buffer
  uint8_t *buf = (uint8_t *)frame;

  // checksum as 4 byte to handle overflows
  uint32_t sum = 0;

  // iterate throught byte pairs and combine them into a 2 byte word
  uint16_t word;
  for (size_t i = 0; i < frame_size; i += 2) {
    if (i + 1 < frame_size) {
      word = (buf[i] << 8) | buf[i + 1];
    } else {
      word = (buf[i] << 8) | 0;
    }

    // add subsum
    sum += word;

    // handle carry
    if (sum > 0xFFFF) {
      sum = (sum & 0xFFFF) + (sum >> 16);
    }
  }

  return (~sum) & 0xFFFF;
}

// return the md5 hash from a string
char *get_md5_str(const char *data) {
  // get md5 digest
  EVP_MD_CTX *ctx = EVP_MD_CTX_new();
  if (!ctx)
    return NULL;

  unsigned char digest[EVP_MAX_MD_SIZE];
  unsigned int digest_len;

  if (EVP_DigestInit_ex(ctx, EVP_md5(), NULL) != 1 ||
      EVP_DigestUpdate(ctx, data, strlen(data)) != 1 ||
      EVP_DigestFinal_ex(ctx, digest, &digest_len) != 1) {
    EVP_MD_CTX_free(ctx);
    return NULL;
  }

  EVP_MD_CTX_free(ctx);

  // Each digest byte will become a 2 char hexa
  // 1 extra byte needed to \0
  unsigned int hash_len = 2 * digest_len;
  char *md5_string = malloc(hash_len + 1);
  if (!md5_string)
    return NULL;

  // iterate through digest bytes
  for (unsigned int i = 0; i < digest_len; ++i)
    // set each 2 str chars as hexa from digest byte
    sprintf(&md5_string[i * 2], "%02x", digest[i]);

  // last str char
  md5_string[hash_len] = '\0';
  return md5_string;
}
