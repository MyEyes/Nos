#ifndef IDT_H
#define IDT_H

#include <stdint.h>

#define DIV_ERROR 0x0
#define SS_DEBUG_EXC 0x1
#define NMI 0x2
#define BRK 0x3
#define OVERFLOW 0x4
#define BOUND_EXC 0x5
#define INV_OP_EXC 0x6
#define NO_COPROC_EXC 0x7
#define DOUBLE_FAULT 0x8
#define COPROC_SEG_OVR_EXC 0x9
#define INVALID_TSS_EXC 0xA 
#define SEGFAULT 0xB
#define STACK_EXC 0xC
#define GENERAL_PROT_FAULT 0xD
#define PAGEFAULT 0xE
#define COPROC_ERR 0x10

#define IRQ_OFFSET 0x20
#define FREE_IRQ_START IRQ_OFFSET+0x10

#define PROC_START 0x40

#define DRV_START 0x80

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
void register_drv_int(uint8_t index, uint32_t offset);
void enable_idt();
void load_idt();
void setup_idt();
#endif