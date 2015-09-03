#include <stdlib.h>
#include <memory.h>
#include <ipc/ipc.h>
#include <string.h>

void std_init()
{
	init_ipc();
	mem_init();
}

extern void (*_fini)();

void abort()
{
	exit(-1);
}

void* malloc(size_t size)
{
	//Find a segment that is big enough
	mem_seg_t* seg = mem_get_free_seg(size);
	//If no segment is found return 0
	//Ideally ask kernel for more memory
	if(seg==0)
		return 0;
	//Split the requested memory out of the found segment
	mem_split_seg(seg, size);
	//Mark segment as used
	seg->free = 0;
	//return the address of the first unused byte after the memory structure
	return (void*)seg + sizeof(mem_seg_t);
}

void* calloc(size_t num, size_t size)
{
	//Todo: handle overflow
	void* ptr = malloc(num*size);
	if(ptr)
		memzero(ptr, num*size);
	return ptr;
}

void free(void *ptr)
{
	if(!ptr)
		return;
	//get the segment corresponding to that address
	mem_seg_t* seg = mem_get_seg(ptr);
	//If no such segment exists we're trying to free something
	//that has either already been freed or was never allocated
	if(seg==0)
		return;
	//If it's not free we free it
	if(!seg->free)
		seg->free = 1;
	else
		//double free
		return;
	//try to merge it into a bigger page with surrounding pages
	mem_try_merge_seg(seg);
}