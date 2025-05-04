// file:        msg-controller.h
// description: definitions for the controller with global variables
#ifndef MSG_CONTROLLER_H
#define MSG_CONTROLLER_H

#include "defs.h"
#include <pthread.h>
#include <stdlib.h>

// dccnet messages controller
typedef struct {
  int waiting_ack;
  uint16_t current_id;
  uint16_t last_sent_id;
  uint16_t last_received_id;
  char last_received_data[MAX_DATA_BYTES];
  size_t last_size_received;
  int received_end;
  int sent_end;
  int new_data_available;
  pthread_mutex_t mc_mutex;
  pthread_cond_t mc_ack_cond;
  pthread_cond_t mc_data_cond;
} MsgController;

// global controller
extern MsgController msg_controller;

// message contoller functions
void init_msg_controller(MsgController *mc);
void set_last_received_data(MsgController *mc, const char *data,
                            size_t data_size);
void clean_msg_controller(MsgController *mc);

#endif
