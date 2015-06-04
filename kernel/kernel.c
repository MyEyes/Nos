#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <task.h>
#include <tss.h>
#include <scheduler.h>

#include <int.h>
#include <idt.h>
#include <pic.h>

#include <meminfo.h>
#include <portio.h>
#include <gdt.h>
#include <paging.h>
#include <kalloc.h>
#include <ipc/ipc.h>

#include <terminal.h>
#include <clock.h>
#include <debug.h>
#include "drv/terminal_drv.h"

#include <kernel.h>


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
	
	init_ipc();
	
}

void kern_test_term_drv()
{
	bochs_break();
	enable_interrupts();
	send_to_port(1, -1, "This is a test!\n", 17);
	send_to_port(1, -1, "If you see this message\n", 25);
	send_to_port(1, -1, "Then rudimentary IPC works\n", 28);
	while(1);
}

void kernel_run()
{
	terminal_writestring("Hello, kernel World!\n\n");
	
	set_idt_desc(IRQ_OFFSET+0x01, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8); //Disable keyboard interrupt	
	
	start_terminal_drv();
	
	task_t* term_test = create_task(kern_test_term_drv, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	scheduler_spawn(term_test);
	
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
	
	kernel_task = create_task(kernel_run, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	kernel_task->time_slice = 0x0000F00000000;
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