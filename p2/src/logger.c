// file:        logger.c
// fescription: definitions for useful loggers
#include "logger.h"
#include <stdlib.h>
#include <string.h>

// logger variables
static LogLevel current_level = LOG_DISABLED;
static FILE *log_file = NULL;

// correct program usage
void usage(const char *program) {
  printf("Usage: %s <address> <period> [startup]\n", program);
  exit(EXIT_FAILURE);
}

// finish program due failure
void log_exit(const char *msg) {
  fprintf(stderr, "%s\n", msg);
  exit(EXIT_FAILURE);
}

// convert a log level to a string
static const char *get_level_str(LogLevel level) {
  switch (level) {
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

char *get_log_file_name(const char *ip) {
  char *folder = "logs/";
  char *name = "_log.txt";
  size_t str_size = strlen(folder) + strlen(ip) + strlen(name);

  char *file_name = malloc(str_size);
  strcpy(file_name, folder);
  strcat(file_name, ip);
  strcat(file_name, name);

  return file_name;
}

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
