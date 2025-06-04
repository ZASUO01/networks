// file:        router.c
// fescription: implementation of router functions
#include "router.h"
#include "cJSON.h"
#include "logger.h"
#include "network.h"
#include <limits.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

// global variable
Router router;

// router params initializer
void init_router(Router *rt, int sock_fd, const char *ip, int period) {
  rt->sock_fd = sock_fd;
  pipe(rt->pipe_fd);
  strcpy(rt->ip, ip);
  rt->period = period;
  rt->operating = 1;
  rt->neighbors_count = 0;
  rt->routes_count = 0;
  pthread_mutex_init(&rt->router_mutex, NULL);
  pthread_cond_init(&rt->router_update_cond, NULL);
}

// returns the neighbor index if it exists
static int find_neighbor(Router *rt, const char *ip) {
  for (int i = 0; i < rt->neighbors_count; i++) {
    if (strcmp(rt->neighbors[i].ip, ip) == 0) {
      return i;
    }
  }
  return -1;
}

// return the best route if it exists
static int find_best_route(Router *rt, const char *dest_ip) {
  int best_cost = INT_MAX;
  int best_id = -1;

  for (int i = 0; i < rt->routes_count; i++) {
    if (strcmp(rt->routes[i].dest_ip, dest_ip) == 0 &&
        rt->routes[i].cost < best_cost) {
      best_cost = rt->routes[i].cost;
      best_id = i;
    }
  }

  return best_id;
}

static int find_single_route(Router *rt, const char *via_ip,
                             const char *dest_ip) {
  for (int i = 0; i < rt->routes_count; i++) {
    if (strcmp(rt->routes[i].dest_ip, dest_ip) == 0 &&
        strcmp(rt->routes[i].via_ip, via_ip) == 0) {
      return i;
    }
  }
  return -1;
}

// add a neighbor and a route to it
void add_neighbor(Router *rt, const char *ip, int weight) {
  pthread_mutex_lock(&rt->router_mutex);

  int idx = find_neighbor(rt, ip);
  if (idx < 0 && rt->neighbors_count < MAX_NEIGHBORS &&
      strcmp(rt->ip, ip) != 0) {
    strcpy(rt->neighbors[rt->neighbors_count].ip, ip);
    rt->neighbors[rt->neighbors_count].weight = weight;
    rt->neighbors[rt->neighbors_count].last_update = time(NULL);
    rt->neighbors_count++;

    strcpy(rt->routes[rt->routes_count].dest_ip, ip);
    strcpy(rt->routes[rt->routes_count].via_ip, ip);
    rt->routes[rt->routes_count].cost = weight;
    rt->routes[rt->routes_count].timestamp = time(NULL);

    if (rt->routes_count < MAX_ROUTES - 1) {
      rt->routes_count++;
    }

    LOG_MSG(LOG_INFO, "add_neighbor(): %s added", ip);
  } else {
    LOG_MSG(LOG_INFO, "add_neighbor(): %s not added", ip);
  }
  pthread_mutex_unlock(&rt->router_mutex);
}

// delete a neighbor and learned routes from it
void del_neighbor(Router *rt, const char *ip) {
  pthread_mutex_lock(&rt->router_mutex);

  // copy the ip as the pointer will be changed below
  char tmp_ip[MAX_IP];
  strcpy(tmp_ip, ip);

  int idx = find_neighbor(rt, tmp_ip);
  if (idx != -1) {
    int i;
    for (i = idx; i < rt->neighbors_count - 1; i++) {
      rt->neighbors[i] = rt->neighbors[i + 1];
    }
    rt->neighbors_count--;

    for (i = 0; i < rt->routes_count;) {
      if (strcmp(rt->routes[i].via_ip, tmp_ip) == 0 ||
          strcmp(rt->routes[i].dest_ip, tmp_ip) == 0) {

        for (int j = i; j < rt->routes_count - 1; j++) {
          rt->routes[j] = rt->routes[j + 1];
        }
        rt->routes_count--;
      } else {
        i++;
      }
    }
    LOG_MSG(LOG_INFO, "del_neighbor(): %s deleted", ip);
  } else {
    LOG_MSG(LOG_INFO, "del_neighbor(): %s not deleted", ip);
  }
  pthread_mutex_unlock(&rt->router_mutex);
}

// send a data message to the destination
static void send_data(Router *rt, cJSON *msg) {
  /// get the last router to send the reply
  cJSON *routers = cJSON_GetObjectItem(msg, "routers");
  int list_size = cJSON_GetArraySize(routers);
  cJSON *last_router = cJSON_GetArrayItem(routers, list_size - 2);
  const char *last_ip = last_router->valuestring;

  // get the trace source
  const char *source = cJSON_GetObjectItem(msg, "source")->valuestring;

  // build the reply message
  cJSON *reply = cJSON_CreateObject();
  cJSON_AddStringToObject(reply, "type", "data");
  cJSON_AddStringToObject(reply, "source", rt->ip);
  cJSON_AddStringToObject(reply, "destination", source);
  cJSON_AddItemToObject(reply, "payload", msg);
  char *resp = cJSON_PrintUnformatted(reply);

  // send data back
  int bytes_sent = send_packet(rt->sock_fd, last_ip, resp, strlen(resp));

  if (bytes_sent == -1) {
    LOG_MSG(LOG_ERROR, "send_data(): reply not sent");
  } else {
    LOG_MSG(LOG_INFO, "send_data(): reply sent");
  }
  cJSON_Delete(reply);
  free(resp);
}

// process a received json data msg
void process_data(Router *rt, cJSON *msg) {
  pthread_mutex_lock(&rt->router_mutex);

  const char *dest = cJSON_GetObjectItem(msg, "destination")->valuestring;
  cJSON *payload = cJSON_GetObjectItem(msg, "payload");

  // this router is the data destination
  if (strcmp(rt->ip, dest) == 0) {
    const char *payload_str = cJSON_Print(payload);
    printf("%s\n", payload_str);
    LOG_MSG(LOG_INFO, "process_data(): data printed");
  }

  // need to foward data back in the routers list
  else {
    cJSON *routers = cJSON_GetObjectItem(payload, "routers");
    int routers_size = cJSON_GetArraySize(routers);

    for (int i = routers_size - 1; i >= 0; i--) {
      cJSON *curr = cJSON_GetArrayItem(routers, i);

      if (strcmp(curr->valuestring, rt->ip) == 0) {
        if (i == 0) {
          LOG_MSG(LOG_ERROR, "process_data(): no router to foward back");
        } else {
          cJSON *prev = cJSON_GetArrayItem(routers, i - 1);
          char *prev_ip = prev->valuestring;

          char *fwd = cJSON_PrintUnformatted(msg);
          char *ffwd = cJSON_Print(msg);

          int bytes_sent = send_packet(rt->sock_fd, prev_ip, fwd, strlen(fwd));

          if (bytes_sent == -1) {
            LOG_MSG(LOG_INFO, "process_data(): data not fowarded back\n%s",
                    ffwd);
          } else {
            LOG_MSG(LOG_INFO, "process_trace(): data fowarded back\n%s", ffwd);
          }
        }
        break;
      }
    }
  }
  pthread_mutex_unlock(&rt->router_mutex);
}

// send a trace json msg to a destination ip
void send_trace(Router *rt, const char *dest_ip) {
  pthread_mutex_lock(&rt->router_mutex);

  int route_id = find_best_route(rt, dest_ip);
  if (route_id != -1) {
    // create trace msg
    cJSON *trace_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(trace_msg, "type", "trace");
    cJSON_AddStringToObject(trace_msg, "source", rt->ip);
    cJSON_AddStringToObject(trace_msg, "destination", dest_ip);

    // list of routers in this route
    cJSON *router_array = cJSON_CreateArray();
    cJSON_AddItemToArray(router_array, cJSON_CreateString(rt->ip));
    cJSON_AddItemToObject(trace_msg, "routers", router_array);

    // json to string
    char *json = cJSON_PrintUnformatted(trace_msg);
    char *formatted_json = cJSON_Print(trace_msg);

    LOG_MSG(LOG_INFO, "send_trace(): trace msg sent\n%s", formatted_json);

    int bytes_sent = send_packet(rt->sock_fd, rt->routes[route_id].via_ip, json,
                                 strlen(json));
    if (bytes_sent < 0) {
      LOG_MSG(LOG_ERROR, "send_trace(): no bytes sent");
    } else {
      LOG_MSG(LOG_ERROR, "send_trace(): %d bytes sent", bytes_sent);
    }

    cJSON_Delete(trace_msg);
    free(json);
    free(formatted_json);
  } else {
    LOG_MSG(LOG_WARNING, "send_trace(): no route found");
  }

  pthread_mutex_unlock(&rt->router_mutex);
}

// process a received trace json msg
void process_trace(Router *rt, cJSON *msg) {
  pthread_mutex_lock(&rt->router_mutex);

  // add this router to the routers list
  cJSON *routers = cJSON_GetObjectItem(msg, "routers");
  cJSON_AddItemToArray(routers, cJSON_CreateString(rt->ip));

  // check if this router is the destination or not
  const char *dest = cJSON_GetObjectItem(msg, "destination")->valuestring;
  if (strcmp(dest, rt->ip) == 0) {
    LOG_MSG(LOG_INFO, "process_trace(): this is the final destination");
    send_data(rt, msg);
  }

  // foward the trace msg
  else {
    int idx = find_best_route(&router, dest);
    if (idx != -1) {
      char *fwd = cJSON_PrintUnformatted(msg);
      char *ffwd = cJSON_PrintUnformatted(msg);

      int bytes_sent =
          send_packet(rt->sock_fd, rt->routes[idx].via_ip, fwd, strlen(fwd));

      if (bytes_sent == -1) {
        LOG_MSG(LOG_INFO, "process_trace(): msg not fowarded\n%s", ffwd);
      } else {
        LOG_MSG(LOG_INFO, "process_trace(): msg fowarded\n%s", ffwd);
      }
      free(fwd);
      free(ffwd);
    } else {
      LOG_MSG(LOG_INFO, "process_trace(): msg not fowarded");
    }
  }
  pthread_mutex_unlock(&rt->router_mutex);
}

// send udated routes to all the neighbors
void send_update(Router *rt) {
  pthread_mutex_lock(&rt->router_mutex);
  for (int i = 0; i < rt->neighbors_count; i++) {
    cJSON *update_msg = cJSON_CreateObject();
    cJSON_AddStringToObject(update_msg, "type", "update");
    cJSON_AddStringToObject(update_msg, "source", rt->ip);
    cJSON_AddStringToObject(update_msg, "destination", rt->neighbors[i].ip);

    // add all known routes to the message
    cJSON *distances = cJSON_CreateObject();
    for (int j = 0; j < rt->routes_count; j++) {
      // only add if the destination is not accessed via the neighbor
      if (strcmp(rt->routes[j].via_ip, rt->neighbors[i].ip) != 0 &&
          strcmp(rt->routes[j].dest_ip, rt->neighbors[i].ip) != 0) {
        // check if the key is already added
        cJSON *dest = cJSON_GetObjectItem(distances, rt->routes[j].dest_ip);
        if (dest && rt->routes[i].cost < dest->valueint) {
          dest->valueint = rt->routes[i].cost;
        } else {
          cJSON_AddNumberToObject(distances, rt->routes[j].dest_ip,
                                  rt->routes[j].cost);
        }
      }
    }
    cJSON_AddNumberToObject(distances, rt->ip, rt->neighbors[i].weight);
    cJSON_AddItemToObject(update_msg, "distances", distances);

    // send the created message
    char *json = cJSON_PrintUnformatted(update_msg);
    char *formatted_json = cJSON_Print(update_msg);

    int bytes_sent =
        send_packet(rt->sock_fd, rt->neighbors[i].ip, json, strlen(json));
    if (bytes_sent == -1) {
      LOG_MSG(LOG_ERROR, "send_update(): failed to send update to ip = %s",
              rt->neighbors[i].ip);
    } else {
      LOG_MSG(LOG_INFO, "send_update(): %d bytes sent to ip = %s\n%s",
              bytes_sent, rt->neighbors[i].ip, formatted_json);
    }

    // clean memory
    cJSON_Delete(update_msg);
    free(json);
    free(formatted_json);
  }
  pthread_mutex_unlock(&rt->router_mutex);
}

// process update json mesage
void process_update(Router *rt, cJSON *msg) {
  pthread_mutex_lock(&rt->router_mutex);

  // get the distances object
  cJSON *distances = cJSON_GetObjectItem(msg, "distances");
  cJSON *dest;

  // check if the sender is already a neighbor and add it case not
  char *sender = cJSON_GetObjectItem(msg, "source")->valuestring;
  int sender_weight;
  int sender_idx = find_neighbor(rt, sender);
  if (sender_idx < 0) {
    cJSON_ArrayForEach(dest, distances) {
      if (strcmp(dest->string, sender) == 0) {
        pthread_mutex_unlock(&rt->router_mutex);
        add_neighbor(rt, dest->string, dest->valueint);
        pthread_mutex_lock(&rt->router_mutex);
        sender_weight = dest->valueint;
        break;
      }
    }
  } else {
    sender_weight = rt->neighbors[sender_idx].weight;
    rt->neighbors[sender_idx].last_update = time(NULL);
  }

  // update or add other routes
  time_t timestamp_now = time(NULL);
  cJSON_ArrayForEach(dest, distances) {
    if (strcmp(dest->string, sender) != 0) {
      int route_idx = find_single_route(rt, sender, dest->string);
      // add route
      if (route_idx < 0) {
        strcpy(rt->routes[rt->routes_count].via_ip, sender);
        strcpy(rt->routes[rt->routes_count].dest_ip, dest->string);
        rt->routes[rt->routes_count].cost = sender_weight + dest->valueint;
        rt->routes[rt->routes_count].timestamp = timestamp_now;

        if (rt->routes_count < MAX_ROUTES - 1) {
          rt->routes_count++;
        }
      }
      // update route
      else {
        rt->routes[route_idx].cost = sender_weight + dest->valueint;
        rt->routes[route_idx].timestamp = timestamp_now;
      }
    }
  }

  // delete obsolete routes
  for (int i = 0; i < rt->routes_count;) {
    if (strcmp(rt->routes[i].via_ip, sender) == 0 &&
        strcmp(rt->routes[i].dest_ip, sender) != 0 &&
        rt->routes[i].timestamp < timestamp_now) {

      for (int j = i; j < rt->routes_count - 1; j++) {
        rt->routes[j] = rt->routes[j + 1];
      }
      rt->routes_count--;
    } else {
      i++;
    }
  }

  LOG_MSG(LOG_INFO, "process_update(): routes updated");
  pthread_mutex_unlock(&rt->router_mutex);
}

// check if the neighbors haven't send updates for a long period
void check_timeouts(Router *rt) {
  time_t now = time(NULL);

  pthread_mutex_lock(&rt->router_mutex);
  // for each neighbor
  for (int i = 0; i < rt->neighbors_count;) {
    // no update since (4 * period) seconds ago
    if ((int)difftime(now, rt->neighbors[i].last_update) > (4 * rt->period)) {
      LOG_MSG(LOG_INFO, "check_timeouts(): removed ip = %s",
              rt->neighbors[i].ip);
      pthread_mutex_unlock(&rt->router_mutex);
      del_neighbor(rt, rt->neighbors[i].ip);
      pthread_mutex_lock(&rt->router_mutex);
    } else {
      i++;
    }
  }
  pthread_mutex_unlock(&rt->router_mutex);
}

// helper function to print router data
void print_info(Router *rt) {
  pthread_mutex_lock(&rt->router_mutex);
  int i;

  printf("ip: %s\n", rt->ip);
  printf("neighbors: [ ");
  for (i = 0; i < rt->neighbors_count; i++) {
    printf("%s ", rt->neighbors[i].ip);
  }
  printf(" ]\n");
  printf("routes: [ ");
  for (i = 0; i < rt->routes_count; i++) {
    printf("(%s - %s) ", rt->routes[i].dest_ip, rt->routes[i].via_ip);
  }
  printf(" ]\n");
  pthread_mutex_unlock(&rt->router_mutex);
}
