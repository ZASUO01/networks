#include <arpa/inet.h>
#include <logger.h>
#include <network.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define TIMEOUT 1

// try to parse ipv4 or ipv6 addr
int parse_addr(const char *addr_str, struct sockaddr_storage *storage) {
  if (addr_str == NULL) {
    return -1;
  }

  // set the port
  uint16_t port = htons(PORT);

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

// create an udp socket and bind it with an ip addr
int create_and_bind_socket(const char *addr_str) {
  // try to parse ip addr
  struct sockaddr_storage storage;
  struct sockaddr *addr = (struct sockaddr *)&storage;
  if (parse_addr(addr_str, &storage) != 0) {
    log_exit("parse addr failure");
  }

  // create udp socket
  int sock_fd = socket(storage.ss_family, SOCK_DGRAM, 0);
  if (sock_fd < 0) {
    log_exit("socket creation failure");
  }

  // bind addr
  if (bind(sock_fd, addr, sizeof(storage)) != 0) {
    log_exit("addr bind failure");
  }

  return sock_fd;
}

// send bytes to an ip destination
int send_packet(int fd, const char *dest_ip, const char *msg, size_t msg_size) {
  // set select params
  fd_set writefds;
  struct timeval timeout;

  FD_ZERO(&writefds);
  FD_SET(fd, &writefds);

  timeout.tv_sec = TIMEOUT;
  timeout.tv_usec = 0;

  // wait the socket to be ready to send
  int ret = select(fd + 1, NULL, &writefds, NULL, &timeout);

  // socket not ready
  if (ret <= 0) {
    return -1;
  }

  // set dest addr
  struct sockaddr_storage storage;
  struct sockaddr *addr = (struct sockaddr *)&storage;
  if (parse_addr(dest_ip, &storage) != 0) {
    return -1;
  }

  // send if socket ready
  if (FD_ISSET(fd, &writefds)) {
    ssize_t bytes_sent = sendto(fd, msg, msg_size, 0, addr, sizeof(storage));
    if (bytes_sent != (ssize_t)msg_size) {
      return -1;
    }

    return bytes_sent;
  }

  return -1;
}

// try to receive a packet
int receive_packet(int fd, int pipe_fd[2], char *msg, size_t msg_size) {
  // init buffer
  memset(msg, 0, msg_size);

  // sender addr
  struct sockaddr_storage storage;
  struct sockaddr *addr = (struct sockaddr *)&storage;
  socklen_t len = sizeof(storage);

  // set socket and timeout for select
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(fd, &readfds);
  FD_SET(pipe_fd[0], &readfds);

  int maxfd = (fd > pipe_fd[0]) ? fd : pipe_fd[0];

  struct timeval timeout;
  timeout.tv_sec = TIMEOUT;
  timeout.tv_usec = 0;

  int retval = select(maxfd + 1, &readfds, NULL, NULL, &timeout);
  // socket not ready
  if (retval <= 0) {
    return -1;
  }

  // manually stoped
  if (FD_ISSET(pipe_fd[0], &readfds)) {
    char temp;
    read(pipe_fd[0], &temp, 1);
    return -1;
  }

  // data available from network
  if (FD_ISSET(fd, &readfds)) {
    ssize_t bytes_count = recvfrom(fd, msg, msg_size, 0, addr, &len);

    if (bytes_count <= 0) {
      return -1;
    }

    return bytes_count;
  }

  return -1;
}
