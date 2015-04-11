#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>
#include "util/terminal.h"
#include "res/meminfo.h"
#include "util/int.h"
#include "res/gdt.h"
#include "util/idt.h"
#include "res/portio.h"
#include "util/clock.h"
#include "util/debug.h"
#include "res/paging.h"
#include "res/kalloc.h"
#include "kernel.h"

void kernel_bootstrap()
{	
	set_meminfo_ptr((meminfo_page_t**)(MEMINFO_LOC));	//Get current RAM info
	
	init_gdt();										//Set up our own GDT
	
	init_kernel_paging();							//Initialize paging
	
	kalloc_vmem_add((void*)KMEM_KERN_RESERVED_LIMIT, 0x100000); //1MB
	
	terminal_initialize();
	
	setup_idt();									//Allocate IDT memory and clear table
	
	clock_init();									//Initialize system clock
	
	load_idt();										//Point IDT and set interrupt table active
}

void kernel_main()
{
	kernel_bootstrap();
	
	terminal_writestring("Hello, kernel World!\n\n");
	print_meminfo();
	
	terminal_writestring("\n");
	print_gdt_info();
	
	terminal_writestring("\n");
	
	set_idt_desc(0x09, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8); //Disable keyboard interrupt

	//enable_interrupts();
	bochs_break();
	while(1);
	halt();
}

void kernel_panic()
{
	terminal_initialize();
	terminal_writestring("Kernel Panic\nEmergency halt\n");
	disable_interrupts();
	halt();
	while(1); //Infinite loop if the halt somehow breaks
}