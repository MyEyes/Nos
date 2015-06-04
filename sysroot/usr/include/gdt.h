#ifndef GDT_H
#define GDT_H

#include <stdint.h>
#include <stddef.h>

#define GDT_ENTRY_COUNT 0x08
#define GDT_KERNEL_CODE_SEG 0x08
#define GDT_KERNEL_DATA_SEG 0x10
#define GDT_KERNEL_TSS_SEG 0x18
#define GDT_USER_CODE_SEG 0x20
#define GDT_USER_DATA_SEG 0x28
#define GDT_USER_TSS_SEG 0x30

enum  __attribute__((__packed__)) gdt_entry_type
{
	GDT_Present = 128,
	GDT_DPL1 = 64,
	GDT_DPL2 = 32,
	GDT_Sys = 16,
	GDT_Code = 8,
	GDT_Conforming =4,
	GDT_ExpDown = 4,
	GDT_RW = 2,
	GDT_ACC = 1
};

enum __attribute__((__packed__)) gdt_flags
{
	GDT_BigPages = 128,
	GDT_Bit32 = 64,
	GDT_GrowDown = 64,
	GDT_Reserved = 32,
	GDT_AVL = 16
};

typedef struct
{
	char l0;
	char l1;
	char b0;
	char b1;
	char b2;
	char TT;
	char Fl;
	char b3;
} gdt_entry_raw_t;

typedef struct
{
	uint32_t base_addr;
	uint32_t size;
	enum gdt_entry_type type;
	enum gdt_flags flags;
} gdt_entry_t;

typedef struct __attribute__((__packed__))
{
	uint16_t size;
	gdt_entry_raw_t* entry_ptr;
} gdt_info_t;

void set_old_gdt(gdt_info_t* ptr);
void print_gdt_info();

void gdt_print_entry(gdt_entry_t entry);
gdt_entry_raw_t gdt_get_raw_entry(uint32_t index);

gdt_entry_t raw_to_gdt_entry(gdt_entry_raw_t entry);
gdt_entry_raw_t gdt_entry_to_raw(gdt_entry_t entry);

void remap_gdt();
void set_gdt_entry(uint8_t index, gdt_entry_t entry);

gdt_entry_t construct_gdt_entry(uint32_t base_addr, uint32_t size, enum gdt_entry_type type, enum gdt_flags flags);

void init_gdt();
#endif