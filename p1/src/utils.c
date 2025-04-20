#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

static LogLevel current_level = LOG_ERROR;
static FILE *log_file = NULL;

static const char *get_level_str(LogLevel level) {
  switch (level) {
  case LOG_DEBUG:
    return "DEBUG";
  case LOG_INFO:
    return "INFO";
  case LOG_WARNING:
    return "WARNING";
  case LOG_ERROR:
    return "ERROR";
  default:
    return "UNKNOWN";
  }
}

// logger setters
void set_log_level(LogLevel level) { current_level = level; }
void set_log_file(FILE *file) { log_file = file; }

// print the log if the current level can be shown
void log_message(LogLevel level, const char *fmt, ...) {
  if (level < current_level) {
    return;
  }

  if (!log_file) {
    log_file = stderr;
  }

  va_list args;
  va_start(args, fmt);
  fprintf(log_file, "%s: ", get_level_str(level));
  vfprintf(log_file, fmt, args);
  va_end(args);
}

// print the correct program usage and finish the program
void usage_md5(const char *program) {
  printf("usage: %s <server IP> <server port> <GAS>\n", program);
  exit(EXIT_FAILURE);
}

// finish the program with an error message
void log_exit(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}
