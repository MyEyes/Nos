#ifndef CLOCK_H
#define CLOCK_H
#include <stdint.h>

void clock_init();
uint64_t clock_get_time();
uint64_t clock_time_diff();
#endif