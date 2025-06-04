
// file:        router.h
// description: definitions of router and router functions
#ifndef ROUTER_H
#define ROUTER_H

#include "cJSON.h"
#include <pthread.h>

#define MAX_IP 64
#define MAX_NEIGHBORS 1000
#define MAX_ROUTES 5000

typedef struct {
  char ip[MAX_IP];
  int weight;
  time_t last_update;
} Neighbor;

typedef struct {
  char dest_ip[MAX_IP];
  char via_ip[MAX_IP];
  int cost;
  time_t timestamp;
} Route;

typedef struct {
  int sock_fd;
  int pipe_fd[2];
  char ip[MAX_IP];
  int period;
  int operating;
  int neighbors_count;
  int routes_count;
  Neighbor neighbors[MAX_NEIGHBORS];
  Route routes[MAX_ROUTES];
  pthread_mutex_t router_mutex;
  pthread_cond_t router_update_cond;
} Router;

extern Router router;

void init_router(Router *rt, int sock_fd, const char *ip, int period);
void add_neighbor(Router *rt, const char *ip, int weight);
void del_neighbor(Router *rt, const char *ip);
void send_trace(Router *rt, const char *dest_ip);
void process_trace(Router *rt, cJSON *msg);
void process_data(Router *rt, cJSON *msg);
void send_update(Router *rt);
void process_update(Router *rt, cJSON *msg);
void check_timeouts(Router *rt);
void print_info(Router *rt);

#endif
