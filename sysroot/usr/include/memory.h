#ifndef MEMORY_H_
#define MEMORY_H_

#include <stddef.h>
#include <stdint.h>
#define PAGE_SIZE 0x1000

typedef struct mem_seg_t_s
{
	struct mem_seg_t_s* 	prev;
	struct mem_seg_t_s* 	next;
	size_t 					size;
	char					free;
} mem_seg_t;

mem_seg_t* 	mem_get_free_seg(size_t);
void		mem_split_seg(mem_seg_t*, size_t);
void		mem_try_merge_seg(mem_seg_t*);
mem_seg_t* 	mem_get_seg(void*);
void 		mem_append_seg(size_t);
void 		mem_init();

#endif