#ifndef COMMON_H
#define COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

#define MAX_LINE_LENGTH 1024
#define MAX_FIELDS 20
#define MAX_FIELD_LENGTH 256

// Helper to remove newline characters
void trim_newline(char *str);

#endif
