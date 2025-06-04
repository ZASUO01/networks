// file:        operations.c
// description: implementation of program main operations
#include "operations.h"
#include "cJSON.h"
#include "logger.h"
#include "network.h"
#include "router.h"
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MAX_INPUT 256
#define MAX_MSG 4096

void startup_router(Router *rt, FILE *startup_file) {
  char line[MAX_INPUT];
  char ip[MAX_IP];
  int weight;

  while (fgets(line, 256, startup_file)) {
    if (strncmp(line, "add ", 4) == 0) {
      if (sscanf(line + 4, "%s %d", ip, &weight) == 2) {
        add_neighbor(rt, ip, weight);
      }
    }
  }
}

// thread to read commands from terminal and execute the correct operation
static void *read_input_thread() {
  LOG_MSG(LOG_INFO, "read_input_thread(): start");

  while (1) {
    // stop condition
    pthread_mutex_lock(&router.router_mutex);
    if (router.operating == 0) {
      pthread_mutex_unlock(&router.router_mutex);
      break;
    }
    pthread_mutex_unlock(&router.router_mutex);

    // read variables
    char cmd[MAX_INPUT];
    char ip[MAX_IP];
    int weight;

    // read from command line
    if (fgets(cmd, MAX_INPUT, stdin) != NULL) {
      // remove line break
      cmd[strcspn(cmd, "\n")] = 0;

      // quit command
      if (strncmp(cmd, "quit", 4) == 0) {
        LOG_MSG(LOG_INFO, "read_input_thread(): quit command");
        pthread_mutex_lock(&router.router_mutex);

        // disable router
        router.operating = 0;

        // wake up select function in receiver thread
        write(router.pipe_fd[1], "x", 1);

        // wake up update thread
        pthread_cond_signal(&router.router_update_cond);
        pthread_mutex_unlock(&router.router_mutex);
      }

      // add command
      else if (strncmp(cmd, "add ", 4) == 0) {
        if (sscanf(cmd + 4, "%s %d", ip, &weight) == 2) {
          LOG_MSG(LOG_INFO, "read_input_thread(): add cmd");
          add_neighbor(&router, ip, weight);
        }
      }

      // del command
      else if (strncmp(cmd, "del ", 4) == 0) {
        if (sscanf(cmd + 4, "%s", ip) == 1) {
          LOG_MSG(LOG_INFO, "read_input_thread(): del cmd");
          del_neighbor(&router, ip);
        }
      }

      // trace command
      else if (strncmp(cmd, "trace ", 6) == 0) {
        if (sscanf(cmd + 6, "%s", ip) == 1) {
          LOG_MSG(LOG_INFO, "read_input_thread(): trace cmd");
          send_trace(&router, ip);
        }
      }

      else if (strncmp(cmd, "print", 5) == 0) {
        LOG_MSG(LOG_INFO, "read_input_thread(): print cmd");
        print_info(&router);
      }
    }
  }

  LOG_MSG(LOG_INFO, "read_input_thread(): stop");
  return NULL;
}

// thread to periodicaly send updated routes to neighbors
static void *send_update_thread(void *arg) {
  LOG_MSG(LOG_INFO, "send_update_thread(): start");
  int *period = (int *)arg;

  while (1) {
    time_t before = time(NULL);
    pthread_mutex_lock(&router.router_mutex);
    if (router.operating == 0) {
      pthread_mutex_unlock(&router.router_mutex);
      break;
    }
    pthread_mutex_unlock(&router.router_mutex);
    time_t after = time(NULL);

    send_update(&router);

    pthread_mutex_lock(&router.router_mutex);
    int diff = (int)difftime(after, before);
    if (*period > diff) {
      struct timespec ts;
      clock_gettime(CLOCK_REALTIME, &ts);
      ts.tv_sec += (*period - diff);

      pthread_cond_timedwait(&router.router_update_cond, &router.router_mutex,
                             &ts);
    }
    pthread_mutex_unlock(&router.router_mutex);
  }

  LOG_MSG(LOG_INFO, "send_update_thread(): stop");
  return NULL;
}

// thread to receive and process json messages
static void *receive_msg_thread() {
  LOG_MSG(LOG_INFO, "receive_msg_thread(): start");

  while (1) {
    check_timeouts(&router);

    pthread_mutex_lock(&router.router_mutex);
    if (router.operating == 0) {
      pthread_mutex_unlock(&router.router_mutex);
      break;
    }
    pthread_mutex_unlock(&router.router_mutex);

    char buf[MAX_MSG];
    int bytes_received =
        receive_packet(router.sock_fd, router.pipe_fd, buf, MAX_MSG);

    if (bytes_received < 0) {
      LOG_MSG(LOG_WARNING, "receive_thread(): no bytes received");
      continue;
    } else {
      cJSON *msg = cJSON_Parse(buf);
      if (!msg) {
        LOG_MSG(LOG_WARNING, "receive_thread(): json parse failed");
        continue;
      }

      const char *type = cJSON_GetObjectItem(msg, "type")->valuestring;

      // update msg
      if (strcmp(type, "update") == 0) {
        LOG_MSG(LOG_INFO, "receive_thread(): received update");
        process_update(&router, msg);
      }

      else if (strcmp(type, "trace") == 0) {
        LOG_MSG(LOG_INFO, "receive_thread(): received trace");
        process_trace(&router, msg);
      }

      else if (strcmp(type, "data") == 0) {
        LOG_MSG(LOG_INFO, "receive_thread(): received data");
        process_data(&router, msg);
      }
    }
  }

  LOG_MSG(LOG_INFO, "receive_msg_thread(): stop");
  return NULL;
}

void execute_operations(int period) {
  // declare threads
  pthread_t input_t, update_t, receive_t;

  // init threads
  pthread_create(&input_t, NULL, read_input_thread, NULL);
  pthread_create(&update_t, NULL, send_update_thread, &period);
  pthread_create(&receive_t, NULL, receive_msg_thread, NULL);

  // wait for threads result
  pthread_join(input_t, NULL);
  pthread_join(update_t, NULL);
  pthread_join(receive_t, NULL);
}
