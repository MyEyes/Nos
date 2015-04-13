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
#include "util/pic.h"
#include "res/tss.h"
#include "util/task.h"
#include "res/scheduler.h"
#include "kernel.h"


void kernel_bootstrap()
{	
	set_meminfo_ptr((meminfo_page_t**)(MEMINFO_LOC));	//Get current RAM info
	
	init_gdt();										//Set up our own GDT
	
	init_kernel_paging();							//Initialize paging
	
	kalloc_vmem_add((void*)KMEM_KERN_RESERVED_LIMIT, KMEM_START_RAM); //1MB
	
	terminal_initialize();
	
	init_kernel_tss();
	
	load_tss(GDT_KERNEL_TSS_SEG);
	
	setup_idt();									//Allocate IDT memory and clear table
	
	clock_init();									//Initialize system clock
	
	load_idt();										//Point IDT and set interrupt table active
	
	remap_pic();
	
	//map_dir(kernel_page_dir, (void*)KMEM_USER_STACK_LOC, (void*) KMEM_USER_STACK_LOC);
}

void mem_violation_test()
{
	
}

void kernel_run()
{
		
	terminal_writestring("Hello, kernel World!\n\n");
	//print_meminfo();

	//pmem_print_info();
	
	terminal_writestring("\n");
	//print_gdt_info();
	
	//terminal_writestring("\n");
	
	set_idt_desc(IRQ_OFFSET+0x01, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8); //Disable keyboard interrupt
	
	enable_interrupts();
	
	kalloc_vmem_add((void*)USERSPACE_LOC, pmem_total_memory-KMEM_KERNEL_LIMIT);
		
	load_tss(GDT_USER_TSS_SEG+3);
	
	task_t* task = create_user_task(mem_violation_test, 0);
	scheduler_spawn(task);
	
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
	
	//load_tss(GDT_KERNEL_TSS_SEG);
	kernel_task = create_task(kernel_run, 0x10, 0x8, 0x10, 0);
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
	gdt_entry_t entry = raw_to_gdt_entry(gdt_get_raw_entry((*(uint16_t*)0x1100)/8));
	print_tss((tss_entry_t*)entry.base_addr);
	halt();
	while(1); //Infinite loop if the halt somehow breaks
}