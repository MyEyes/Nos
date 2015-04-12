#include "clock.h"
#include "terminal.h"
#include "idt.h"
#include "../res/portio.h"

uint32_t clock_fractions;
uint32_t clock_ms;

uint32_t system_timer_ms;
uint32_t system_timer_fractions;

extern void IRQ0_handler();


void clock_init()
{
	uint16_t reload_value = 0x1000;
	
	//Set up IRQ0 interrupt handler
	//set_idt_desc(IRQ_OFFSET+0x00, (uint32_t)&IRQ0_handler, 0, IntGate32, 0x8);
	outb(0x43, 0b00110100); //Channel 0, lobyte/hibyte, rate generator
	outb(0x40, reload_value&0xFF);
	outb(0x40, (reload_value>>8)&0xFF);
	uint64_t clock_time = (reload_value * 3000 * 4398046511104 /*2^42*/ /3579545) / 1024;
	clock_fractions = clock_time & 0xFFFFFFFF;
	clock_ms = (clock_time>>32) & 0xFFFFFFFF;
	system_timer_ms = 0;
	system_timer_fractions = 0;
}

uint64_t clock_get_time()
{
	uint64_t val = system_timer_ms & 0xFFFFFFFF;
	val <<= 32;
	val |= system_timer_fractions & 0xFFFFFFFF;
	return val;
}