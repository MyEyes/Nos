#include <lock.h>

uint32_t acquire_lock(lock_t* lock)
{
	//Try to get the lock until you get it
	while(!__sync_bool_compare_and_swap(&lock->locked,0,1));
	
	return 1;
}

void release_lock(lock_t* lock)
{
	//This will free the lock
	lock->locked = 0;
}