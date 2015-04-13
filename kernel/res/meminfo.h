#ifndef MEMINFO_H
#define MEMINFO_H
#include <stdint.h>

#define PMEM_KERNEL_OWNER 0xFF00
//We store extra info in the PMEM pointers
//These macros help to get that
#define PMEM_ADDR(x) ((pmem_table_t*)((uint32_t)x & 0xFFFFF000))
#define PMEM_INFO(x) (uint16_t)((uint32_t)x & 0xFFF)
#define PMEM_IADDR(x,y) ((pmem_table_t*)(((uint32_t)x&0xFFFFF000) | ((uint16_t)y & 0xFFF)))

typedef struct
{
	uint64_t base_addr;
	uint64_t reg_length;
	uint32_t reg_type;
	uint32_t acpi_ext;
} meminfo_page_t;

typedef enum
{
	PMEM_PRESENT = 1,
	PMEM_RESERVED = 2,
	PMEM_KERN = 4,
	PMEM_MAPPED = 128
} __attribute__((packed)) pmem_entry_flags;

typedef struct
{
	uint16_t owner;
	pmem_entry_flags flags;
	uint8_t reserved;
} __attribute__((packed)) pmem_table_entry_t;

typedef struct
{
	pmem_table_entry_t entries[1024];
} pmem_table_t;

typedef struct
{
	pmem_table_t* table;
} pmem_dir_entry_t;

typedef struct
{
	pmem_dir_entry_t entries[1024];
} pmem_dir_t;

void set_meminfo_ptr(meminfo_page_t** firstPage);
void init_meminfo();
void print_meminfo();
void print_meminfo_page(meminfo_page_t* page);
meminfo_page_t* get_free_meminfo_page(uint8_t index);
void pmem_mod_range(uint32_t base_addr, uint32_t size, uint16_t owner, pmem_entry_flags flags);
void pmem_mod(uint32_t addr, uint16_t owner, pmem_entry_flags flags);
void pmem_create_table(uint32_t addr);
void pmem_print_info();
void pmem_map();
void* pmem_get_free_page();
#endif