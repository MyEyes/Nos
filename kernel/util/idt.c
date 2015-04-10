#include "idt.h"

idt_desc_t idt_descs[256];
idt_info_t idt_info __asm__ ("idt_info");

void enable_idt()
{
	//__asm__  __volatile__ ("xchg %bx, %bx");
	//ptr = &idt_info;
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

void setup_idt()
{
	for(uint16_t x=0; x<256; x++)
	{
		set_idt_desc(x, 0, 0, 0, 0);
	}
	set_idt_desc(0x80, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8);
}