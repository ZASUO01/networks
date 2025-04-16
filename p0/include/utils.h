#ifndef UTILS_H
#define UTILS_H

#include <stdarg.h>
#include <stdio.h>

// simple logger
typedef enum {
  LOG_DEBUG,
  LOG_INFO,
  LOG_WARNING,
  LOG_ERROR,
} LogLevel;

#define LOG_MSG(level, fmt, ...) log_message(level, fmt, ##__VA_ARGS__)

void usage(const char *program);
void log_exit(const char *msg);
void set_log_level(LogLevel level);
void set_log_file(FILE *file);
void log_message(LogLevel level, const char *fmt, ...);

#endif
