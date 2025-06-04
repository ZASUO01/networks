// file:        operations.h
// description: definitions of program main operations
#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "router.h"
#include <stdio.h>

void startup_router(Router *rt, FILE *startup_file);
void execute_operations(int period);

#endif
