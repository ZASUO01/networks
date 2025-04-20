// file:        parser.h
// description: definitions for loggers
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

void log_exit(const char *msg);
void usage_md5(const char *program);
void usage_xfer(const char *program);
void set_log_level(LogLevel level);
void set_log_file(FILE *file);
void log_message(LogLevel level, const char *fmt, ...);

#endif
