#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define IRQ_OFFSET 0x20
#define FREE_IRQ_START IRQ_OFFSET+0x10

extern void do_nothing_int();

typedef enum
{
	TaskGate32 = 0x5,
	IntGate16 = 0x6,
	TrapGate16 = 0x7,
	IntGate32 = 0xE,
	TrapGate32 = 0xF
} idt_type;

typedef struct __attribute__((__packed__))
{
	uint16_t offset_1;
	uint16_t selector;
	uint8_t zero;
	uint8_t type_attr;
	uint16_t offset_2; 
} idt_desc_t;

typedef struct __attribute__((__packed__))
{
	uint16_t size;
	idt_desc_t* ptr;
} idt_info_t;

void set_idt_desc(uint8_t index, uint32_t offset, uint8_t cpulevel, idt_type type, uint16_t selector);
void enable_idt();
void load_idt();
void setup_idt();
#endif