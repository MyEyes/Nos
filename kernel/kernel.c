#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
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
#include "drv/ext2.h"
#include <drv/devio.h>
#include <floppy.h>

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

void kern_test_floppy_drv()
{
	enable_interrupts();
	floppy_init();
	
	dev_desc_t floppy_desc;
	floppy_desc.name[0] = 'f';
	floppy_desc.name[1] = 'l';
	floppy_desc.name[2] = 'o';
	floppy_desc.name[3] = 'p';
	floppy_desc.name[4] = 'p';
	floppy_desc.name[5] = 'y';
	floppy_desc.read_op = floppy_read;
	
	ext2_hook_t ext2_hook = ext2_create_hook(&floppy_desc);
	
	terminal_writestring("\n");
	terminal_writestring("\n");
	if(!ext2_hook.valid)
		terminal_writestring("Not a valid ext2 file system\n");
	else
		terminal_writestring("Found valid ext2 file system\n");
	
	if(ext2_read_inode(&ext2_hook, 2)<0)
		terminal_writestring("Error reading root directory\n");
	
	char* buffer = (char*) kalloc(1024);
	ext2_read_inode_content(&ext2_hook, 2, 0, buffer, 1024);
	uint16_t offset = 0;
	int inode = -1;
	
	while(offset<1024)
	{
		ext2_dir_entry_t* curr_dir = (ext2_dir_entry_t*)(buffer+offset);
		terminal_writeuint16(offset);
		terminal_writeuint32(curr_dir->inode);
		terminal_writeuint16(curr_dir->total_size);
		terminal_writestring_l(&curr_dir->name, curr_dir->name_len);
		terminal_writestring("\n");
		if(curr_dir->name == 't')
			inode = curr_dir->inode;
		offset+=curr_dir->total_size;
	}
	terminal_writestring("\n");
	ext2_read_inode_content(&ext2_hook, inode, 0, buffer, 1024);
	bochs_break();
	terminal_writestring_l(buffer, 100);
	
	uint64_t curr_time = clock_get_time();
	while(1)
	{
		uint64_t ct = clock_get_time();
		if(ct-curr_time>0x40000000000)
		{
			bochs_break();
			curr_time = ct;
		}
	}
}

void kern_test_term_drv()
{
	bochs_break();
	enable_interrupts();
	print("This is a test!\n");
	print("If you see this message\n");
	print("Then rudimentary IPC works\n");
	while(1);
}

void kernel_run()
{
	terminal_writestring("Hello, kernel World!\n\n");
	
	set_idt_desc(IRQ_OFFSET+0x01, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8); //Disable keyboard interrupt	
	
	start_terminal_drv();
	
	task_t* floppy_test = create_task(kern_test_floppy_drv, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	schd_task_add(floppy_test);
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