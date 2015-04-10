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
#include "kernel.h"

void kernel_bootstrap()
{
	terminal_initialize();
	
	set_meminfo_ptr((meminfo_page_t**)(MEMINFO_LOC));	//Get current RAM info
	
	init_gdt();
	
	setup_idt();									//Allocate IDT memory and clear table
	
	clock_init();									//Initialize system clock
	
	load_idt();										//Point IDT and set interrupt table active
	
	//Map kernel memory
	kernel_page_dir = page_dir_create(0);
	page_dir_create((void*)PAGE_SIZE);
	page_dir_entry_create(kernel_page_dir->entries, (void*)PAGE_SIZE, PG_RW|PG_Present);
	
	for(uint32_t index = 0; index<1024; index++)
	{
		map_dir(kernel_page_dir, (void*)(index*PAGE_SIZE), (void*)(index*PAGE_SIZE));
	}
	bochs_break();
	enable_paging(kernel_page_dir);
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