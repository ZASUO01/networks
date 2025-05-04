#include "server-client.h"
#include "logger.h"
#include "msg-controller.h"
#include "network.h"
#include "operations.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

// try to connect the client with a server and return its socket
int init_and_connect_client(const char *addr_str, const char *port_str) {
  LOG_MSG(LOG_INFO, "init_and_connect_client(): start");

  // try to parse port and addr
  struct sockaddr_storage storage;
  if (parse_addr(addr_str, port_str, &storage) != 0) {
    log_exit("failed to parse addr and port");
  }
  struct sockaddr *server_addr = (struct sockaddr *)&storage;

  // init TCP socket
  int sock_fd = socket(storage.ss_family, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    log_exit("failed to create socket");
  }

  // try to connect to server
  if (connect(sock_fd, server_addr, sizeof(storage)) != 0) {
    log_exit("failed to connect to the server");
  }

  LOG_MSG(LOG_INFO, "init_and_connect_client(): complete");
  return sock_fd;
}

// init server with given port and protocol and
int init_server(const char *protocol, const char *port_str) {
  LOG_MSG(LOG_INFO, "init_server(): start");

  // try to init server addr
  struct sockaddr_storage storage;
  if (init_server_addr(protocol, port_str, &storage) != 0) {
    log_exit("server addr failure");
  }
  struct sockaddr *addr = (struct sockaddr *)&storage;

  // create TCP socket
  int sock_fd = socket(storage.ss_family, SOCK_STREAM, 0);
  if (sock_fd < 0) {
    log_exit("socket failure");
  }

  // try to bind socket
  if (bind(sock_fd, addr, sizeof(storage)) != 0) {
    log_exit("bind failure");
  }

  // try to linsten on socket
  if (listen(sock_fd, 10) != 0) {
    log_exit("listen failure");
  }

  LOG_MSG(LOG_INFO, "init_server(): complete");
  return sock_fd;
}

// exchange file lines with a client
void server_actions(int fd, FILE *input, FILE *output) {
  LOG_MSG(LOG_INFO, "server_actions(): start");

  while (1) {
    LOG_MSG(LOG_INFO, "server_actions(): waiting for connection...");

    // client addr
    struct sockaddr_storage client_storage;
    socklen_t addr_size = sizeof(client_storage);
    struct sockaddr *client_addr = (struct sockaddr *)&client_storage;

    // accept client connection
    int client_fd = accept(fd, client_addr, &addr_size);
    if (client_fd < 0) {
      continue;
    }

    LOG_MSG(LOG_INFO, "client connected");

    // declare threads
    pthread_t recv_t, send_t, print_t;

    // thread arguments
    ThreadArgs *tr = (ThreadArgs *)malloc(sizeof(ThreadArgs));
    tr->fd = client_fd;
    tr->input = input;
    tr->output = output;

    // create threads
    pthread_create(&send_t, NULL, send_xfer_thread, tr);
    pthread_create(&recv_t, NULL, receive_thread, tr);
    pthread_create(&print_t, NULL, print_thread, tr);

    // wait for thread result
    pthread_join(send_t, NULL);
    pthread_join(recv_t, NULL);
    pthread_join(print_t, NULL);

    free(tr);
    close(client_fd);
    break;
  }
}

// receive lines from the server and return with md5 hash
void client_md5_actions(int fd, const char *gas, size_t gas_size,
                        FILE *output) {
  LOG_MSG(LOG_INFO, "client_md5_actions(): start");

  // this client doesn't send any data lines
  msg_controller.sent_end = 1;

  // declare threads
  pthread_t send_t, recv_t;

  // set thread arguments
  ThreadArgs *tr = (ThreadArgs *)malloc(sizeof(ThreadArgs));
  tr->fd = fd;
  tr->gas = strdup(gas);
  tr->gas_size = gas_size;
  tr->output = output;

  // create threads
  pthread_create(&send_t, NULL, send_md5_thread, tr);
  pthread_create(&recv_t, NULL, receive_thread, tr);

  // wait for threads result
  pthread_join(send_t, NULL);
  pthread_join(recv_t, NULL);

  free(tr->gas);
  free(tr);
  LOG_MSG(LOG_INFO, "client_md5_actions(): complete");
}

// exchange file lines with a server
void client_xfer_actions(int fd, FILE *input, FILE *output) {
  LOG_MSG(LOG_INFO, "client_xfer_actions(): start"); // declare threads
  pthread_t recv_t, send_t, print_t;

  // thread arguments
  ThreadArgs *tr = (ThreadArgs *)malloc(sizeof(ThreadArgs));
  tr->fd = fd;
  tr->input = input;
  tr->output = output;

  // create threads
  pthread_create(&send_t, NULL, send_xfer_thread, tr);
  pthread_create(&recv_t, NULL, receive_thread, tr);
  pthread_create(&print_t, NULL, print_thread, tr);

  // wait for thread result
  pthread_join(send_t, NULL);
  pthread_join(recv_t, NULL);
  pthread_join(print_t, NULL);

  free(tr);
  close(fd);
  LOG_MSG(LOG_INFO, "client_xfer_actions(): complete");
}
