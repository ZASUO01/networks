#ifndef DEFS_H
#define DEFS_H

#include <stdint.h>

// error byte sizes
#define RESPONSE_ERROR_LENGHT 4
#define ERROR_CODE_BYTE_SIZE 2

// error codes
#define INVALID_MESSAGE_CODE 1
#define INCORRECT_MESSAGE_LENGTH 2
#define INVALID_PARAMETER 3
#define INVALID_SINGLE_TOKEN 4
#define ASCII_DECODE_ERROR 5

// message byte sizes
#define TYPE_BYTE_SIZE 2
#define ID_BYTE_SIZE 12
#define NONCE_BYTE_SIZE 4
#define TOKEN_BYTE_SIZE 64
#define STATUS_BYTE_SIZE 1
#define SAS_NUM_BYTE_SIZE 2
#define SAS_BYTE_SIZE 80

// response types
#define ITR_REQUEST_TYPE 1
#define ITR_RESPONSE_TYPE 2
#define ITV_REQUEST_TYPE 3
#define ITV_RESPONSE_TYPE 4
#define GTR_REQUEST_TYPE 5
#define GTR_RESPONSE_TYPE 6
#define GTV_REQUEST_TYPE 7
#define GTV_RESPONSE_TYPE 8

// comunication attempts
#define MAX_ATTEMPTS 6

#endif
