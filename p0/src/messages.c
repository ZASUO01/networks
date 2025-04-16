#include "defs.h"
#include "utils.h"
#include <arpa/inet.h>
#include <ctype.h>
#include <messages.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

// aux function to remove empty chars from a string
static void trim_str(char *str, size_t len) {
  char *end = str + len - 1;
  while (end > str && isspace((unsigned char)*end)) {
    end--;
  }

  *(end + 1) = '\0';
}

// aux function to get the correspondent error message
static char *get_error_message(unsigned int error_code) {
  switch (error_code) {
  case INVALID_MESSAGE_CODE:
    return "Error: Request sent with an unknown type";
    break;
  case INCORRECT_MESSAGE_LENGTH:
    return "Error: Request size is incompatible with the request type";
    break;
  case INVALID_PARAMETER:
    return "Error: Request fields error";
    break;
  case INVALID_SINGLE_TOKEN:
    return "Error: A SAS in the request is invalid";
    break;
  case ASCII_DECODE_ERROR:
    return "Error: Either Id or Token have invalid characters";
    break;
  default:
    return "Error: Unknown error happened";
    break;
  }
}

// try to send request and receive response
static int send_receive(int fd, char *req, size_t req_size, char *res,
                        size_t res_size) {
  LOG_MSG(LOG_INFO, "send_receive(): init\n");
  //  attempts to send request and receive reponse
  int max_attempts = MAX_ATTEMPTS;
  int total_attempts = 0;
  ssize_t bytes_count;

  while (total_attempts < max_attempts) {
    total_attempts++;
    LOG_MSG(LOG_INFO, "send_receive(): attempt %d\n", total_attempts);

    // try to send the request
    bytes_count = send(fd, req, req_size, 0);
    if (bytes_count != (ssize_t)req_size) {
      LOG_MSG(LOG_ERROR, "send_receive(): message send failure\n");
      return -1;
    }
    LOG_MSG(LOG_INFO, "send_receive(): %ld bytes sent successfully\n",
            bytes_count);

    // try to receive the response
    bytes_count = recv(fd, res, res_size, 0);

    // if any byte is received
    if (bytes_count > 0) {
      // error bytes
      if (bytes_count == RESPONSE_ERROR_LENGHT) {
        LOG_MSG(LOG_ERROR, "send_receive(): got response error\n");

        size_t byte_offset = ERROR_CODE_BYTE_SIZE;
        uint16_t error_code;
        memcpy(&error_code, res + byte_offset, sizeof(error_code));
        return ntohs(error_code);
      }
      // got response
      else {
        LOG_MSG(LOG_INFO, "send_receive(): %ld bytes received successfully\n",
                bytes_count);
        return 0;
      }
    }
    LOG_MSG(LOG_WARNING, "send_receive(): no response yet, retry...\n");
  }
  // no response from server
  LOG_MSG(LOG_ERROR, "send_receive(): no response from server\n");
  return -1;
}

// individual token request operation
char *itr_operation(int fd, const char *id, uint32_t nonce) {
  LOG_MSG(LOG_INFO, "itr_operation(): init\n");
  // request structure
  size_t req_size = TYPE_BYTE_SIZE + ID_BYTE_SIZE + NONCE_BYTE_SIZE;
  char request[req_size];

  // init bytes
  memset(request, 0, req_size);
  size_t byte_offset = 0;

  // set type
  uint16_t _type = htons(ITR_REQUEST_TYPE);
  memcpy(request, &_type, sizeof(_type));
  byte_offset += sizeof(_type);

  // set id
  memset(request + byte_offset, ' ', ID_BYTE_SIZE);
  memcpy(request + byte_offset, id, strlen(id));
  byte_offset += ID_BYTE_SIZE;

  // set nonce
  uint32_t _nonce = htonl(nonce);
  memcpy(request + byte_offset, &_nonce, sizeof(_nonce));

  // response structure
  size_t res_size = req_size + TOKEN_BYTE_SIZE;
  char response[res_size];
  memset(response, 0, res_size);

  // try to send request and receive response
  int result = send_receive(fd, request, req_size, response, res_size);

  // failure
  if (result < 0) {
    LOG_MSG(LOG_ERROR, "itr_operation(): exit without response\n");
    return NULL;
  }

  // response error
  if (result > 0) {
    char *msg = NULL;

    if (asprintf(&msg, "%s", get_error_message(result)) < 0) {
      LOG_MSG(LOG_ERROR, "itr_operation(): reponse conversion failed\n");
      return NULL;
    }

    LOG_MSG(LOG_ERROR, "itr_operation(): exit with error message\n");
    return msg;
  }

  // got response
  memcpy(&_type, response, sizeof(_type));
  _type = ntohs(_type);

  // check valid type
  if (_type != ITR_RESPONSE_TYPE) {
    LOG_MSG(LOG_ERROR, "itr_operation(): invalid response type\n");
    return NULL;
  }

  // copy response id
  byte_offset = sizeof(_type);
  char _id[ID_BYTE_SIZE];
  memcpy(_id, response + byte_offset, ID_BYTE_SIZE);
  trim_str(_id, ID_BYTE_SIZE);
  byte_offset += ID_BYTE_SIZE;

  // copy response nonce
  memcpy(&_nonce, response + byte_offset, sizeof(_nonce));
  _nonce = ntohl(_nonce);
  byte_offset += sizeof(_nonce);

  // copy response token
  char _token[TOKEN_BYTE_SIZE + 1];
  memcpy(_token, response + byte_offset, TOKEN_BYTE_SIZE);
  _token[TOKEN_BYTE_SIZE] = '\0';

  char *msg = NULL;
  if (asprintf(&msg, "%s:%u:%s", _id, nonce, _token) < 0) {
    LOG_MSG(LOG_ERROR, "itr_operation(): reponse conversion failed\n");
    return NULL;
  }

  LOG_MSG(LOG_INFO, "itr_operation(): exit with message\n");
  return msg;
}

// individual token validation operation
// returns 0 if the token is valid and 1 otherwise
int itv_operation(int fd, const char *sas) {
  LOG_MSG(LOG_INFO, "itv_operation(): init\n");

  // request structure
  size_t req_size =
      TYPE_BYTE_SIZE + ID_BYTE_SIZE + NONCE_BYTE_SIZE + TOKEN_BYTE_SIZE;
  char request[req_size];

  // init bytes
  memset(request, 0, req_size);
  size_t byte_offset = 0;

  // set type
  uint16_t _type = htons(ITV_REQUEST_TYPE);
  memcpy(request, &_type, sizeof(_type));
  byte_offset += sizeof(_type);

  // parse SAS
  char _id[ID_BYTE_SIZE + 1];
  uint32_t _nonce;
  char _token[TOKEN_BYTE_SIZE + 1];

  if (sscanf(sas, "%12[^:]:%u:%64s", _id, &_nonce, _token) != 3) {
    LOG_MSG(LOG_ERROR, "itv_operation(): SAS parsing failure\n");
    return 1;
  };

  _id[ID_BYTE_SIZE] = '\0';
  _nonce = htonl(_nonce);
  _token[TOKEN_BYTE_SIZE] = '\0';

  // set id
  memset(request + byte_offset, ' ', ID_BYTE_SIZE);
  memcpy(request + byte_offset, _id, strlen(_id));
  byte_offset += ID_BYTE_SIZE;

  // set nonce
  memcpy(request + byte_offset, &_nonce, sizeof(_nonce));
  byte_offset += sizeof(_nonce);

  // set token
  memcpy(request + byte_offset, _token, TOKEN_BYTE_SIZE);

  // response structure
  size_t res_size = req_size + STATUS_BYTE_SIZE;
  char response[res_size];
  memset(response, 0, res_size);

  // try to send the request and receive the response
  int result = send_receive(fd, request, req_size, response, res_size);

  // failure
  if (result < 0) {
    LOG_MSG(LOG_ERROR, "itv_operation(): exit without response");
    return 1;
  }

  // error
  if (result > 0) {
    LOG_MSG(LOG_ERROR, "itv_operation(): %s\n", get_error_message(result));
    return 1;
  }

  // got response
  memcpy(&_type, response, sizeof(_type));
  _type = ntohs(_type);

  // check valid type
  if (_type != ITV_RESPONSE_TYPE) {
    LOG_MSG(LOG_ERROR, "itv_operation(): invalid response type\n");
    return 1;
  }

  // check invalid
  byte_offset = req_size;
  uint8_t invalid;

  memcpy(&invalid, response + byte_offset, sizeof(invalid));

  if (invalid > 0) {
    LOG_MSG(LOG_WARNING, "itv_operation(): invalid SAS\n");
    return 1;
  }

  LOG_MSG(LOG_INFO, "itv_operation(): exit with 0\n");
  return 0;
}

// send a list of SAS and return a GAS
char *gtr_operation(int fd, char **sas_list, int n) {
  LOG_MSG(LOG_INFO, "gtr_operation(): init\n");

  // request structure
  size_t req_size = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE + (n * SAS_BYTE_SIZE);
  char request[req_size];

  // init bytes
  memset(request, 0, req_size);
  size_t byte_offset = 0;

  // set type
  uint16_t _type = htons(GTR_REQUEST_TYPE);
  memcpy(request, &_type, sizeof(_type));
  byte_offset += sizeof(_type);

  // set n
  uint16_t _n = htons(n);
  memcpy(request + byte_offset, &_n, sizeof(_n));
  byte_offset += sizeof(_n);

  // parse SAS list
  char _id[ID_BYTE_SIZE + 1];
  uint32_t _nonce;
  char _token[TOKEN_BYTE_SIZE + 1];

  for (int i = 0; i < n; i++) {
    // parse SAS
    if (sscanf(sas_list[i], "%12[^:]:%u:%64s", _id, &_nonce, _token) != 3) {
      LOG_MSG(LOG_ERROR, "gtr_operation(): failed to parse SAS %d\n", i);
      return NULL;
    }

    _id[ID_BYTE_SIZE] = '\0';
    _token[TOKEN_BYTE_SIZE] = '\0';
    _nonce = htonl(_nonce);

    // set id
    memset(request + byte_offset, ' ', ID_BYTE_SIZE);
    memcpy(request + byte_offset, _id, strlen(_id));
    byte_offset += ID_BYTE_SIZE;

    // set nonce
    memcpy(request + byte_offset, &_nonce, sizeof(_nonce));
    byte_offset += sizeof(_nonce);

    // set token
    memcpy(request + byte_offset, _token, TOKEN_BYTE_SIZE);
    byte_offset += TOKEN_BYTE_SIZE;
  }

  // response structure
  size_t res_size = req_size + TOKEN_BYTE_SIZE;
  char response[res_size];

  // try to send te request and receive the response
  int result = send_receive(fd, request, req_size, response, res_size);

  // failure
  if (result < 0) {
    LOG_MSG(LOG_ERROR, "gtr_operation(): exit without response\n");
    return NULL;
  }

  // error
  if (result > 0) {
    char *msg = NULL;

    if (asprintf(&msg, "%s", get_error_message(result)) < 0) {
      LOG_MSG(LOG_ERROR, "gtr_operation(): reponse conversion failed\n");
      return NULL;
    }
    LOG_MSG(LOG_ERROR, "gtr_operation(): exit with error message\n");

    return msg;
  }

  // got response
  memcpy(&_type, response, sizeof(_type));
  _type = ntohs(_type);

  // check valid type
  if (_type != GTR_RESPONSE_TYPE) {
    LOG_MSG(LOG_ERROR, "gtr_operation(): invalid response type\n");
    return NULL;
  }

  byte_offset = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE;

  // allocate all SAS size including ":" and "+"
  char *message = calloc((res_size - byte_offset) + (n * 3), sizeof(char));
  char *tmp = NULL;

  // copy the response SAS list and build the GAS
  for (int i = 0; i < n; i++) {
    // copy id
    memcpy(_id, response + byte_offset, ID_BYTE_SIZE);
    trim_str(_id, ID_BYTE_SIZE);
    byte_offset += ID_BYTE_SIZE;
    _id[ID_BYTE_SIZE] = '\0';

    // copy nonce
    memcpy(&_nonce, response + byte_offset, sizeof(_nonce));
    byte_offset += sizeof(_nonce);
    _nonce = ntohl(_nonce);

    // copy token
    memcpy(_token, response + byte_offset, TOKEN_BYTE_SIZE);
    byte_offset += TOKEN_BYTE_SIZE;
    _token[TOKEN_BYTE_SIZE] = '\0';

    // parse SAS
    if (asprintf(&tmp, "%s:%u:%s", _id, _nonce, _token) < 0) {
      LOG_MSG(LOG_ERROR, "gtr_operation(): SAS %d conversion failed\n", i);
      return NULL;
    }

    // concatenate with previous SAS
    strcat(message, tmp);
    strcat(message, "+");
  }

  free(tmp);

  // concatenate the token at the end
  memcpy(_token, response + byte_offset, TOKEN_BYTE_SIZE);
  _token[TOKEN_BYTE_SIZE] = '\0';
  strcat(message, _token);

  LOG_MSG(LOG_INFO, "gtr_operation(): exit with message\n");
  return message;
}

// group token valitation
// return 0 if the GAS is valid and 1 otherwise
int gtv_operation(int fd, char *gas) {
  LOG_MSG(LOG_INFO, "gtv_operation(): init\n");

  // count the number of SAS in a GAS
  int count = 0;

  for (int i = 0; gas[i] != '\0'; i++) {
    if (gas[i] == '+')
      count++;
  }

  // request structure
  size_t req_size = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE +
                    (count * SAS_BYTE_SIZE) + TOKEN_BYTE_SIZE;
  char request[req_size];

  // init bytes
  memset(request, 0, req_size);
  size_t byte_offset = 0;

  // set type
  int16_t _type = htons(GTV_REQUEST_TYPE);
  memcpy(request, &_type, sizeof(_type));
  byte_offset += sizeof(_type);

  // set n
  int16_t _n = htons(count);
  memcpy(request + byte_offset, &_n, sizeof(_n));
  byte_offset += sizeof(_n);

  // parse GAS
  char _id[ID_BYTE_SIZE + 1];
  int32_t _nonce;
  char _token[TOKEN_BYTE_SIZE + 1];

  char *tmp = strtok(gas, "+");

  // copy SAS list to buffer
  for (int i = 0; i < count; i++) {
    // parse SAS
    if (sscanf(tmp, "%12[^:]:%u:%64s", _id, &_nonce, _token) != 3) {
      LOG_MSG(LOG_ERROR, "gtv_operation(): failed to parse SAS %d\n", i);
      return 1;
    }

    _id[ID_BYTE_SIZE] = '\0';
    _nonce = htonl(_nonce);
    _token[TOKEN_BYTE_SIZE] = '\0';

    // set id
    memset(request + byte_offset, ' ', ID_BYTE_SIZE);
    memcpy(request + byte_offset, _id, strlen(_id));
    byte_offset += ID_BYTE_SIZE;

    // set nonce
    memcpy(request + byte_offset, &_nonce, sizeof(_nonce));
    byte_offset += sizeof(_nonce);

    // set token
    memcpy(request + byte_offset, _token, TOKEN_BYTE_SIZE);
    byte_offset += TOKEN_BYTE_SIZE;

    tmp = strtok(NULL, "+");
  }

  // set the last token
  strncpy(_token, tmp, TOKEN_BYTE_SIZE);
  _token[TOKEN_BYTE_SIZE] = '\0';
  memcpy(request + byte_offset, _token, TOKEN_BYTE_SIZE);

  // response structure
  size_t res_size = req_size + STATUS_BYTE_SIZE;
  char response[res_size];

  // try  to send the request and receive the response
  int result = send_receive(fd, request, req_size, response, res_size);

  // failure
  if (result < 0) {
    LOG_MSG(LOG_ERROR, "gtv_operation(): exit without response\n");
    return 1;
  }

  if (result > 0) {
    LOG_MSG(LOG_ERROR, "gtv_operation(): %s\n", get_error_message(result));
    return 1;
  }

  // got response
  memcpy(&_type, response, sizeof(_type));
  _type = ntohs(_type);

  // check valid type
  if (_type != GTV_RESPONSE_TYPE) {
    LOG_MSG(LOG_ERROR, "gtv_operation(): invalid response type\n");
    return 1;
  }

  byte_offset = TYPE_BYTE_SIZE + SAS_NUM_BYTE_SIZE + (count * SAS_BYTE_SIZE) +
                TOKEN_BYTE_SIZE;
  int8_t valid;
  memcpy(&valid, response + byte_offset, sizeof(valid));

  if (valid > 0) {
    LOG_MSG(LOG_WARNING, "gtv_operation(): invalid GAS\n");
    return 1;
  }

  LOG_MSG(LOG_INFO, "gtv_operation(): exit with 0\n");
  return 0;
}
