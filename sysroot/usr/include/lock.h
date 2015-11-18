#ifndef LOCK_H
#define LOCK_H
#include <stdint.h>

typedef struct
{
	uint32_t locked;
	uint32_t owner_pid;
} lock_t;

uint32_t acquire_lock(lock_t* lock);
uint32_t acquire_lock_spin(lock_t* lock);
uint32_t acquire_lock_blocking(lock_t* lock);
void release_lock(lock_t* lock);

#endif