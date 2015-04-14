#include "idt.h"
#include "terminal.h"
#include "debug.h"
idt_desc_t idt_descs[256];					//Interrupt descriptor table
idt_info_t idt_info __asm__ ("idt_info");	//Interrupt table info

void enable_idt()
{
	__asm__  __volatile__ ("lidt idt_info");
}

void set_idt_desc(uint8_t index, uint32_t offset, uint8_t cpulevel, idt_type type, uint16_t selector)
{
	idt_descs[index].offset_1 = offset & 0xFFFF;
	idt_descs[index].offset_2 = (offset>>16) & 0xFFFF;
	idt_descs[index].selector = selector;
	idt_descs[index].type_attr = type;
	idt_descs[index].type_attr |= (cpulevel&0x03)<<5; //Set cpu level
	idt_descs[index].type_attr |= 0x80; //Set present bit
}

void load_idt()
{
	idt_info.ptr = idt_descs;
	idt_info.size = sizeof(idt_desc_t)*256-1;
	enable_idt();
}

extern void* kernel_panic_handler;

extern void* page_fault_handler;
extern void* spawn;


void setup_idt()
{
	for(uint16_t x=0; x<256; x++)
	{
		set_idt_desc(x, 0, 0, 0, 0);
	}
	
	set_idt_desc(PROC_START, (uint32_t)&spawn, 0, IntGate32, 0x8);
	
	set_idt_desc(SS_DEBUG_EXC, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	set_idt_desc(INVALID_TSS_EXC, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	set_idt_desc(NO_COPROC_EXC, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	set_idt_desc(OVERFLOW, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	set_idt_desc(NMI, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	
	set_idt_desc(DOUBLE_FAULT, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	set_idt_desc(SEGFAULT, (uint32_t)&kernel_panic_handler, 0, IntGate32, 0x8);
	
	set_idt_desc(PAGEFAULT, (uint32_t)&page_fault_handler, 0, IntGate32, 0x8);
}