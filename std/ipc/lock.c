#include <lock.h>
#include <unistd.h>

uint32_t acquire_lock(lock_t* lock)
{	
	return acquire_lock_blocking(lock);
}

uint32_t acquire_lock_spin(lock_t* lock)
{
	//Try to get the lock until you get it
	while(!__sync_bool_compare_and_swap(&lock->locked,0,1));
	
	return 1;
}

uint32_t acquire_lock_blocking(lock_t* lock)
{
	while(!__sync_bool_compare_and_swap(&lock->locked,0,1))
		sleep();
	return 1;
}

void release_lock(lock_t* lock)
{
	//This will free the lock
	lock->locked = 0;
}