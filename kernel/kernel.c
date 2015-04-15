#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>

#include "proc/task.h"
#include "proc/tss.h"
#include "proc/scheduler.h"

#include "int/int.h"
#include "int/idt.h"
#include "int/pic.h"

#include "res/meminfo.h"
#include "res/portio.h"
#include "res/gdt.h"
#include "res/paging.h"
#include "res/kalloc.h"
#include "res/mem.h"

#include "util/terminal.h"
#include "util/clock.h"
#include "util/debug.h"

#include "kernel.h"


void kernel_bootstrap()
{	
	set_meminfo_ptr((meminfo_page_t**)(MEMINFO_LOC));	//Get current RAM info
	
	init_gdt();										//Set up our own GDT
	
	init_kernel_paging();							//Initialize paging
	
	kalloc_init();
	
	terminal_initialize();							//Initialize terminal so we can print debug stuff
	
	init_kernel_tss();								//Initialize kernel context
	
	load_tss(GDT_KERNEL_TSS_SEG);					//Load kernel context
	
	setup_idt();									//Allocate IDT memory and clear table
	
	clock_init();									//Initialize system clock
	
	load_idt();										//Point IDT and set interrupt table active
	
	remap_pic();									//Remap PIC so we can tell IRQs and Exceptions apart
	
}


void kernel_run()
{
	terminal_writestring("Hello, kernel World!\n\n");
	
	set_idt_desc(IRQ_OFFSET+0x01, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8); //Disable keyboard interrupt
		
	load_tss(GDT_USER_TSS_SEG+3);
	
	
	bochs_break();
	terminal_writestring("Back in the kernel\n\n");
	while(1);
	halt();
}

task_t* kernel_task;

void kernel_main()
{	
	create_kernel_context();
	kernel_bootstrap();
	
	kernel_task = create_task(kernel_run,0,0, 0x10, 0x8, 0x10, 0);
	kernel_task->time_slice = 0xFFFF00000000;
	init_scheduler();
	scheduler_spawn(kernel_task);
}

void kernel_panic()
{
	bochs_break();
	disable_interrupts();
	terminal_initialize();
	terminal_writestring("Kernel Panic\n\n");
	
	print_current_task();
	halt();
	while(1); //Infinite loop if the halt somehow breaks
}