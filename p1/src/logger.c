#include "logger.h"
#include <stdlib.h>

static LogLevel current_level = LOG_DISABLED;
static FILE *log_file = NULL;

// convert a log level to a string
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
  // only log allowed levels
  if (level < current_level) {
    return;
  }

  // default log file
  if (log_file == NULL) {
    log_file = stderr;
  }

  // print formated string
  va_list args;
  va_start(args, fmt);
  fprintf(log_file, "%s: ", get_level_str(level));
  vfprintf(log_file, fmt, args);
  fprintf(log_file, "\n");
  va_end(args);
}

// print the correct md5 program usage and finish program
void usage_md5(const char *program) {
  printf("usage: %s <IP>:<PORT> <GAS>\n", program);
  exit(EXIT_FAILURE);
}

// print the correct xfer program usage and finish program
void usage_xfer(const char *program) {
  printf("usage 1: %s -s <PORT> <INPUT> <OUTPUT>\n", program);
  printf("usage 2: %s -c <IP>:<PORT> <INPUT> <OUTPUT>\n", program);
  exit(EXIT_FAILURE);
}

// finish the program with an error message
void log_exit(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}
