#pragma once
#include <sys/time.h>

typedef enum {
    false,
    true
} bool;

int ceiling(float x);

u_int64_t bit(unsigned n);

u_int8_t getBit(int byte, int b);

void printString(char *str);

void displayStats(struct timeval begin, struct timeval end);
