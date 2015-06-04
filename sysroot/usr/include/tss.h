#ifndef TSS_H
#define TSS_H
#include <stdint.h>

typedef struct __attribute__((packed))
{
	uint16_t 	link;
	uint16_t 	res1; //Reserved
	uint32_t 	esp0;
	uint16_t	ss0;
	uint16_t	res2; //Reserved
	uint32_t	esp1;
	uint16_t	ss1;
	uint16_t	res3; //Reserved
	uint32_t	esp2;
	uint16_t	ss2;
	uint16_t	res4; //Reserved
	uint32_t	cr3;
	uint32_t	eip;
	uint32_t	eflags;
	uint32_t	eax;
	uint32_t	ecx;
	uint32_t	edx;
	uint32_t	ebx;
	uint32_t	esp;
	uint32_t	ebp;
	uint32_t	esi;
	uint32_t	edi;
	uint16_t	es;
	uint16_t	res5; //Reserved
	uint16_t	cs;
	uint16_t	res6; //Reserved
	uint16_t	ss;
	uint16_t	res7; //Reserved
	uint16_t	ds;
	uint16_t	res8; //Reserved
	uint16_t	fs;
	uint16_t	res9; //Reserved
	uint16_t	gs;
	uint16_t	res10;//Reserved
	uint16_t	ldtr;
	uint16_t	res11;//Reserved
	uint16_t	res12;//Reserved
	uint16_t	iopb_offset;
} tss_entry_t;

extern tss_entry_t* kernel_tss;
void init_kernel_tss();
void load_tss(uint32_t desc);
void print_tss(tss_entry_t* entry);
#endif