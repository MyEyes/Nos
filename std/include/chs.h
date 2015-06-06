#ifndef _CHS_H
#define _CHS_H
#include <stdint.h>
#include <stddef.h>

typedef struct
{
	uint8_t head;
	uint8_t cylinder;
	uint8_t sector;
} chs_addr_t;

typedef struct
{
	uint16_t num_heads;
	uint16_t num_cylinders;
	uint16_t num_sectors;
	size_t blocksize;
}chs_info_t;

chs_addr_t logical_to_chs(void*, chs_info_t);
void* chs_to_logical(chs_addr_t, chs_info_t);
#endif