#ifndef KALLOC_H
#define KALLOC_H
#include <stdint.h>

typedef struct kalloc_info_st kalloc_info_t;

typedef enum 
{
	KI_free=1,
	KI_reserved=3,
	KI_DMA=5,
} kalloc_info_state;

struct kalloc_info_st
{
	void* base_addr;
	uint32_t size;
	kalloc_info_state state;
	kalloc_info_t* prev;
	kalloc_info_t* next;
};

void kalloc_vmem_add(void* addr, uint32_t size);

kalloc_info_t* kalloc_vmem_block(void* addr, uint32_t size);

void kalloc_print_vmem_info();

void kalloc_print_info(kalloc_info_t* info);

kalloc_info_t* kalloc_create_info(void* addr, uint32_t size, kalloc_info_state state);

kalloc_info_t* kalloc_info_find(kalloc_info_t* start, void* addr);

//size must be 4kb aligned
kalloc_info_t* kalloc_info_split(kalloc_info_t* entry, void* addr, uint32_t size);

void kalloc_info_append(kalloc_info_t* old, kalloc_info_t* new);

void kalloc_info_delete(kalloc_info_t* entry);

kalloc_info_t* kalloc_info_trymerge(kalloc_info_t* original, kalloc_info_t* tryout);

void* kalloc(uint32_t);

void* kalloc_reqDMA(void* addr, uint32_t size);

void kfree(void*);

#endif