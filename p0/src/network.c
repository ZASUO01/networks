#include "network.h"
#include "defs.h"
#include "utils.h"
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

// aux function to get the correspondent error message
static char *get_error_message(unsigned int error_code) {
  switch (error_code) {
  case INVALID_MESSAGE_CODE:
    return "Request sent with an unknown type";
    break;
  case INCORRECT_MESSAGE_LENGTH:
    return "Request size is incompatible with the request type";
    break;
  case INVALID_PARAMETER:
    return "Request fields error";
    break;
  case INVALID_SINGLE_TOKEN:
    return "A SAS in the request is invalid";
    break;
  case ASCII_DECODE_ERROR:
    return "Either Id or Token have invalid characters";
    break;
  default:
    return "Unknown error happened";
    break;
  }
}

// try to send a request and get a reponse from server
static int send_receive(int sock_fd, const char *request, size_t req_size,
                        char *response, size_t res_size) {
  // perform 3 attempts to communicate to the server
  int max_attempts = 3;
  int total_attempts = 0;

  while (total_attempts < max_attempts) {
    total_attempts++;

    // try to send the request
    ssize_t bytes_count = send(sock_fd, request, req_size, 0);
    if (bytes_count != (ssize_t)req_size) {
      log_exit("Send request failure");
    }

    bytes_count = recv(sock_fd, response, res_size, 0);

    // response failure
    if (bytes_count <= 0) {
      if (total_attempts == max_attempts) {
        return -1;
      }
    }

    // response error
    else if (bytes_count == RESPONSE_ERROR_LENGHT) {
      int16_t error_code;
      memcpy(&error_code, response + TYPE_BYTE_SIZE, ERROR_CODE_BYTE_SIZE);
      error_code = ntohs(error_code);
      return error_code;
    }
    // response success
    else {
      return 0;
    }
  }

  return 0;
}

// individual token communication
char *send_receive_itr(int sock_fd, const char *id, int nonce) {
  // result message
  char *message = NULL;

  // request structure
  size_t req_size = TYPE_BYTE_SIZE + ID_BYTE_SIZE + NONCE_BYTE_SIZE;
  char request[req_size];

  // set request fields
  int16_t type = htons(1);
  int32_t _nonce = htonl(nonce);
  char _id[13];
  memset(_id, 0, sizeof(_id));
  strncpy(_id, id, sizeof(_id) - 1);
  _id[ID_BYTE_SIZE] = '\0';

  size_t offset = 0;
  memcpy(request, &type, sizeof(type));
  offset += sizeof(type);

  memcpy(request + offset, _id, sizeof(_id) - 1);
  offset += sizeof(_id) - 1;

  memcpy(request + offset, &_nonce, sizeof(_nonce));

  // response structure
  size_t res_size = req_size + TOKEN_BYTE_SIZE;
  char response[res_size];

  // try to send the request and receive the response
  int result = send_receive(sock_fd, request, sizeof(request), response,
                            sizeof(response));

  // failure
  if (result < 0) {
    return NULL;
  }

  // success
  else if (result == 0) {
    offset = TYPE_BYTE_SIZE;

    memcpy(_id, response + offset, ID_BYTE_SIZE);
    offset += ID_BYTE_SIZE;
    _id[ID_BYTE_SIZE] = '\0';

    memcpy(&_nonce, response + offset, NONCE_BYTE_SIZE);
    offset += NONCE_BYTE_SIZE;
    _nonce = ntohl(_nonce);

    char token[TOKEN_BYTE_SIZE + 1];
    memcpy(token, response + offset, TOKEN_BYTE_SIZE);
    token[TOKEN_BYTE_SIZE] = '\0';

    if (asprintf(&message, "%s:%d:%s", _id, _nonce, token) < 0) {
      log_exit("Server response conversion failure");
    }
  }

  // error
  else {
    if (asprintf(&message, "%s", get_error_message(result)) < 0) {
      log_exit("Server response conversion failure");
    }
  }

  return message;
}

// individual token validation communication
char *send_receive_itv(int sock_fd, const char *sas) {
  // result message
  char *message;

  // request structure
  size_t req_size = TYPE_BYTE_SIZE + SAS_BYTE_SIZE;
  char request[req_size];

  // set request fields
  int16_t type = htons(3);
  char id[13];
  memset(id, 0, sizeof(id));
  int32_t nonce;
  char token[65];

  // parse the given SAS
  if (sscanf(sas, "%12[^:]:%d:%64s", id, &nonce, token) != 3) {
    log_exit("SAS to request conversion failure");
  };

  id[ID_BYTE_SIZE] = '\0';
  token[TOKEN_BYTE_SIZE] = '\0';
  nonce = htonl(nonce);

  // add the fields to the request
  size_t offset = 0;
  memcpy(request, &type, sizeof(type));
  offset += sizeof(type);

  memcpy(request + offset, id, sizeof(id) - 1);
  offset += sizeof(id) - 1;

  memcpy(request + offset, &nonce, sizeof(nonce));
  offset += sizeof(nonce);

  memcpy(request + offset, token, sizeof(token) - 1);

  // response structure
  size_t res_size = req_size + STATUS_BYTE_SIZE;
  char response[res_size];

  int result = send_receive(sock_fd, request, sizeof(request), response,
                            sizeof(response));

  // failure
  if (result < 0) {
    return NULL;
  }

  // success
  else if (result == 0) {
    offset = TYPE_BYTE_SIZE + SAS_BYTE_SIZE;
    int8_t status;

    memcpy(&status, response + offset, STATUS_BYTE_SIZE);

    if (asprintf(&message, "%d", status) < 0) {
      log_exit("Server response conversion failure");
    }
  }

  // error
  else {
    if (asprintf(&message, "%s", get_error_message(result)) < 0) {
      log_exit("Server response conversion failure");
    }
  }

  return message;
}

// group token request communication
char *send_receive_gtr(int sock_fd, char **sas_list, int n) {
  // result message
  char *message;

  // request structure
  size_t req_size = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE + (n * SAS_BYTE_SIZE);
  char request[req_size];

  // fields to be added
  int16_t type = htons(5);
  int16_t N = htons(n);
  char id[13];
  int32_t nonce;
  char token[65];

  // copy type to buffer
  size_t offset = 0;
  memcpy(request, &type, sizeof(type));
  offset += sizeof(type);

  // copy N to buffer
  memcpy(request + offset, &N, sizeof(N));
  offset += sizeof(N);

  // copy SAS list to buffer
  for (int i = 0; i < n; i++) {
    memset(id, 0, sizeof(id));

    // parse SAS
    if (sscanf(sas_list[i], "%12[^:]:%d:%64s", id, &nonce, token) != 3) {
      log_exit("Invalid SAS.");
    }

    id[ID_BYTE_SIZE] = '\0';
    token[TOKEN_BYTE_SIZE] = '\0';
    nonce = htonl(nonce);

    // copy id to buffer
    memcpy(request + offset, id, sizeof(id) - 1);
    offset += sizeof(id) - 1;

    // copy nonce to buffer
    memcpy(request + offset, &nonce, sizeof(nonce));
    offset += sizeof(nonce);

    // copy token to buffer
    memcpy(request + offset, token, sizeof(token) - 1);
    offset += sizeof(token) - 1;
  }

  // response structure
  size_t res_size = req_size + TOKEN_BYTE_SIZE;
  char response[res_size];

  int result = send_receive(sock_fd, request, sizeof(request), response,
                            sizeof(response));

  // response failure
  if (result < 0) {
    message = NULL;
  }
  // response success
  else if (result == 0) {
    offset = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE;

    // allocate all SAS size including ":" and "+"
    message = calloc((res_size - offset) + (n * 3), sizeof(char));
    char *tmp = NULL;

    for (int i = 0; i < n; i++) {
      memcpy(id, response + offset, sizeof(id) - 1);
      offset += sizeof(id) - 1;
      id[ID_BYTE_SIZE] = '\0';

      memcpy(&nonce, response + offset, sizeof(nonce));
      offset += sizeof(nonce);
      nonce = ntohl(nonce);

      memcpy(token, response + offset, sizeof(token) - 1);
      offset += sizeof(token) - 1;
      token[TOKEN_BYTE_SIZE] = '\0';

      if (asprintf(&tmp, "%s:%d:%s", id, nonce, token) < 0) {
        log_exit("Reponse parsing failure");
      }

      strcat(message, tmp);
      strcat(message, "+");
    }

    free(tmp);

    memcpy(token, response + offset, sizeof(token) - 1);
    strcat(message, token);
  }

  // response error
  else {
    if (asprintf(&message, "%s", get_error_message(result)) < 0) {
      log_exit("Server response conversion failure");
    }
  }

  return message;
}

// group token validation communication
char *send_receive_gtv(int sock_fd, char *gas) {
  // result message
  char *message;

  // count the number of SAS in a GAS
  int count = 0;

  for (int i = 0; gas[i] != '\0'; i++) {
    if (gas[i] == '+')
      count++;
  }

  // request structure
  int req_size = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE + (count * SAS_BYTE_SIZE) +
                 TOKEN_BYTE_SIZE;

  char request[req_size];

  // fields to be added
  int16_t type = htons(7);
  int16_t N = htons(count);
  char id[13];
  int32_t nonce;
  char token[65];

  // copy type to buffer
  size_t offset = 0;
  memcpy(request, &type, sizeof(type));
  offset += sizeof(type);

  // copy N to buffer
  memcpy(request + offset, &N, sizeof(N));
  offset += sizeof(N);

  char *tmp = strtok(gas, "+");

  // copy SAS list to buffer
  for (int i = 0; i < count; i++) {
    memset(id, 0, sizeof(id));

    // parse SAS
    if (sscanf(tmp, "%12[^:]:%d:%64s", id, &nonce, token) != 3) {
      log_exit("Invalid SAS.");
    }

    id[ID_BYTE_SIZE] = '\0';
    token[TOKEN_BYTE_SIZE] = '\0';
    nonce = htonl(nonce);

    // copy id to buffer
    memcpy(request + offset, id, sizeof(id) - 1);
    offset += sizeof(id) - 1;

    // copy nonce to buffer
    memcpy(request + offset, &nonce, sizeof(nonce));
    offset += sizeof(nonce);

    // copy token to buffer
    memcpy(request + offset, token, sizeof(token) - 1);
    offset += sizeof(token) - 1;

    tmp = strtok(NULL, "+");
  }

  strncpy(token, tmp, sizeof(token) - 1);
  token[TOKEN_BYTE_SIZE] = '\0';
  memcpy(request + offset, token, sizeof(token) - 1);

  // response structure
  size_t res_size = req_size + 1;
  char response[res_size];

  int result = send_receive(sock_fd, request, sizeof(request), response,
                            sizeof(response));

  // response failure
  if (result < 0) {
    message = NULL;
  }

  // response success
  else if (result == 0) {
    offset = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE + (count * SAS_BYTE_SIZE) +
             TOKEN_BYTE_SIZE;
    int8_t status;
    memcpy(&status, response + offset, sizeof(status));

    if (asprintf(&message, "%d", status) < 0) {
      log_exit("Server response conversion failure");
    }

  }

  // response error
  else {
    if (asprintf(&message, "%s", get_error_message(result)) < 0) {
      log_exit("Server response conversion failure");
    }
  }

  return message;
}
