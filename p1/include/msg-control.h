// file:        msg-control.h
// description: definitions for the controller with global variables
#ifndef MSG_CONTROL_H
#define MSG_CONTROL_H

#include "defs.h"
#include <stdint.h>
#include <stdlib.h>

// dcc net messages control
typedef struct {
  int recv_end;
  int waiting_ack;
  uint16_t current_id;
  uint16_t last_id;
  uint16_t last_recv_id;
  char last_recv_data[MAX_DATA_BYTES];
} MsgControl;

// global variable
extern MsgControl msg_control;

void init_msg_control(MsgControl *ms);
void set_last_recv_id(MsgControl *ms, uint16_t id);
void set_recv_end(MsgControl *ms);
void set_waiting_ack(MsgControl *ms);
void set_next_id(MsgControl *ms);
void set_last_data(MsgControl *ms, const char *data, size_t data_size);

#endif
