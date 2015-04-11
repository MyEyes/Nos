#ifndef MEMINFO_H
#define MEMINFO_H
#include <stdint.h>
typedef struct
{
	uint64_t base_addr;
	uint64_t reg_length;
	uint32_t reg_type;
	uint32_t acpi_ext;
} meminfo_page_t;

void set_meminfo_ptr(meminfo_page_t** firstPage);
void init_meminfo();
void print_meminfo();
void print_meminfo_page(meminfo_page_t* page);
meminfo_page_t* get_free_meminfo_page(uint8_t index);
#endif