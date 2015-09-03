#include <stdlib.h>
#include <memory.h>

mem_seg_t* base_seg;

void mem_init()
{
	base_seg->prev = 0;
	base_seg->next = 0;
	base_seg->size = 2*PAGE_SIZE;
	base_seg->free = 1;
}

mem_seg_t* mem_get_free_seg(size_t size)
{
	mem_seg_t* curr = base_seg;
	do
	{
		if(curr->size >= size + sizeof(mem_seg_t) && curr->free)
		{
			return curr;
		}
		curr = curr->next;
	}while(curr);
	return 0;
}

void mem_split_seg(mem_seg_t* seg, size_t size)
{
	//Check that we have enough space
	if(seg->size<size+sizeof(mem_seg_t))
		return;
	
	//How much is remaining for the second part of this segment
	int remainder = seg->size - size;
	//Set this segment to the new size
	seg->size = size + sizeof(mem_seg_t);
	
	//If there is remaining space create a
	//new node and insert it after this one
	if(remainder>0)
	{
		mem_seg_t* next = seg->next;
	
		seg->next = (mem_seg_t*)((void*)seg+seg->size);
	
		seg->next->next = next;
		if(next)
			next->prev = seg->next;
		seg->next->prev = seg;
		seg->next->size = remainder;
		seg->next->free = seg->free;
	}
}

mem_seg_t* mem_get_seg(void* addr)
{
	uint32_t uiaddr = (uint32_t) addr;
	mem_seg_t* curr = base_seg;
	do
	{
		if((uint32_t)curr == uiaddr-sizeof(mem_seg_t))
			return curr;
		curr = curr->next;
	}while(curr);
	return 0;
}

void mem_append_seg(size_t size)
{
	mem_seg_t* curr = base_seg;
	
	//Go to end of list
	while(curr->next) curr = curr->next;
	
	if(curr->free)
	{
		curr->size += size;
	}
	else
	{
		curr->next = (mem_seg_t*)((void*)curr+curr->size);
		curr->next->prev = curr;
		curr->next->next = 0;
		curr->next->size = size;
		curr->next->free = 1;
	}
}

void mem_try_merge_seg(mem_seg_t* seg)
{
	//If this segment isn't free we can't merge
	if(!seg->free)
		return;
	
	//If the previous segment is free we merge them
	if(seg->prev && seg->prev->free)
	{
		seg->prev->size += seg->size;
		seg->prev->next = seg->next;
		seg = seg->prev;
	}
	//If the next segment is free we merge them
	if(seg->next && seg->next->free)
	{
		seg->size += seg->next->size;
		seg->next = seg->next->next;
	}
}