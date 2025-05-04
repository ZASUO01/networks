#include "msg-controller.h"
#include <pthread.h>
#include <string.h>

MsgController msg_controller;

// init all global variables
void init_msg_controller(MsgController *mc) {
  mc->waiting_ack = 0;
  mc->current_id = 0;
  mc->last_sent_id = -1;
  mc->last_received_id = -1;
  memset(mc->last_received_data, 0, MAX_DATA_BYTES);
  mc->received_end = 0;
  mc->sent_end = 0;
  mc->new_data_available = 0;
  mc->last_size_received = 0;
  pthread_mutex_init(&mc->mc_mutex, NULL);
  pthread_cond_init(&mc->mc_ack_cond, NULL);
  pthread_cond_init(&mc->mc_data_cond, NULL);
}

// copy data to the last received data global variable
void set_last_received_data(MsgController *mc, const char *data,
                            size_t data_size) {
  memset(mc->last_received_data, 0, MAX_DATA_BYTES);
  memcpy(mc->last_received_data, data, data_size);
}

// destroy mutex and conditions
void clean_msg_controller(MsgController *mc) {
  pthread_mutex_destroy(&mc->mc_mutex);
  pthread_cond_destroy(&mc->mc_ack_cond);
  pthread_cond_destroy(&mc->mc_data_cond);
}
