#include "msg-control.h"
#include "defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MsgControl msg_control;

void init_msg_control(MsgControl *ms) {
  ms->recv_end = 0;
  ms->waiting_ack = 0;
  ms->current_id = 0;
  ms->last_id = -1;
  ms->last_recv_id = -1;
  memset(ms->last_recv_data, 0, MAX_DATA_BYTES);
}

void set_last_recv_id(MsgControl *ms, uint16_t id) { ms->last_recv_id = id; }
void set_recv_end(MsgControl *ms) { ms->recv_end = ms->recv_end == 0 ? 1 : 0; }
void set_waiting_ack(MsgControl *ms) {
  ms->waiting_ack = ms->waiting_ack == 0 ? 1 : 0;
}

void set_next_id(MsgControl *ms) {
  ms->last_id = ms->current_id;
  ms->current_id = ms->current_id == 0 ? 1 : 0;
}

void set_last_data(MsgControl *ms, const char *data, size_t data_size) {
  memset(ms->last_recv_data, 0, MAX_DATA_BYTES);
  strncpy(ms->last_recv_data, data, data_size);
  ms->last_recv_data[data_size] = '\0';
}
