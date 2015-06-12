#ifndef _ELF_H
#define _ELF_H
#include <stdint.h>
#include <task.h>

#define ELF_MAGIC_WORD 0x464C457F

typedef struct
{
	uint32_t 	magic;
	uint8_t 	bits;
	uint8_t 	endian;
	uint8_t 	version;
	uint8_t 	target_os;
	uint8_t 	abi_version;
	char		unused[7];
	uint16_t	type;
	uint16_t	target_machine;
	uint32_t	version2;
	uint32_t	entry;
	uint32_t	ph_start;
	uint32_t	sh_start;
	uint32_t	eflags;
	uint16_t	header_size;
	uint16_t	prog_head_size;
	uint16_t	prog_head_count;
	uint16_t	sect_head_size;
	uint16_t	sect_head_count;
	uint16_t	sect_name_entry;
} elf_header_t;

typedef struct
{
	uint32_t	type;
	uint32_t	offset;
	uint32_t	vaddr;
	uint32_t	paddr;
	uint32_t	filesz;
	uint32_t	memsz;
	uint32_t	flags;
	uint32_t	align;
} elf_pheader_t;

task_t* elf_create_proc(elf_header_t* elf_header);

#endif