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
#include "drv/keyboard_drv.h"
#include <drv/devio.h>
#include <floppy.h>
#include <elf.h>

#include "drv/ata.h"

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
	int res = floppy_init();
	
	if(res<0)
		print("There was an issue initializing the floppy driver\n");
	
	dev_desc_t floppy_desc;
	floppy_desc.name[0] = 'f';
	floppy_desc.name[1] = 'l';
	floppy_desc.name[2] = 'o';
	floppy_desc.name[3] = 'p';
	floppy_desc.name[4] = 'p';
	floppy_desc.name[5] = 'y';
	floppy_desc.dev_struct = 0;
	floppy_desc.read_op = floppy_read;
	
	ata_dev_t* ata_dev = ata_init(ATA_PRIMARY_BUS_PORT, ATA_PRIMARY_BUS_DEVICE_CONTROL_PORT, 0, 1024);
	dev_desc_t ata_desc;
	ata_desc.name[0] = 'h';
	ata_desc.name[1] = 'd';
	ata_desc.name[2] = 'd';
	ata_desc.name[3] = 0;
	ata_desc.dev_struct = ata_dev;
	ata_desc.read_op = ata_read;
	
	ext2_hook_t ext2_hook = ext2_create_hook(&floppy_desc);
	
	if(!ext2_hook.valid)
	{
		print("Not a valid ext2 file system\n");
	}
	
	if(ext2_read_inode(&ext2_hook, 2)<0)
		print("Error reading root directory\n");
	
	char* buffer = (char*) kalloc(1024*16);
	ext2_read_inode_content(&ext2_hook, 2, 0, buffer, 1024);
	uint16_t offset = 0;
	int inode = -1;
	
	while(offset<1024)
	{
		ext2_dir_entry_t* curr_dir = (ext2_dir_entry_t*)(buffer+offset);
		if(curr_dir->name == 'c')
			inode = curr_dir->inode;
		offset+=curr_dir->total_size;
	}
	ext2_read_inode_content(&ext2_hook, inode, 0, buffer, 1024*16);
	task_t* task = elf_create_proc((elf_header_t*)buffer);
	//print("Starting usermode task\n");
	scheduler_spawn(task);
	
	uint64_t curr_time = clock_get_time();
	while(1)
	{
		uint64_t ct = clock_get_time();
		if(ct-curr_time>0x40000000000)
		{
			curr_time = ct;
		}
	}
}

void kernel_run()
{
	//terminal_writestring("Hello, kernel World!\n\n");
	set_idt_desc(IRQ_OFFSET+0x01, (uint32_t)&do_nothing_int, 0, IntGate32, 0x8); //Disable keyboard interrupt	
	
	//test_ata();
	start_terminal_drv();
	keyboard_drv_start();
	
	//task_t* ata_test = create_task(per_test, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	//schd_task_add(ata_test);
	
	task_t* floppy_test = create_task(kern_test_floppy_drv, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	scheduler_spawn(floppy_test);
	
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
	kernel_task->time_slice = 0xFFFFFF00000000;
	
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