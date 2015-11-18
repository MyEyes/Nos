#include <task.h>
#include <tss.h>
#include <gdt.h>
#include <kalloc.h>
#include <paging.h>
#include <gdt.h>
#include <terminal.h>
#include <kernel.h>
#include <string.h>

void (*target)();
uint32_t old_stack;
uint32_t target_stack;
uint16_t pid_counter = 0;

//All define in ring3.s
extern void jump_usermode();
extern void switch_context();
extern void switch_context_nolevel();
extern void no_switch();

//We should be in usermode here
void user_mode_entry()
{
	target();
}

task_t* create_user_task(void (*entry)(),void* memstart, void* memend, int8_t priority)
{
	return create_task(entry, memstart,memend, GDT_USER_DATA_SEG+3, GDT_USER_CODE_SEG+3, GDT_USER_DATA_SEG+3, priority);
}

page_dir_t* create_task_pagedir(void* memstart, void*  memend, task_t* task)
{
	uint32_t vmem_size = (sizeof(page_dir_t)+PAGE_SIZE-1)&0xFFFFF000;
	
	//Memory from start to end of pages that the task will need
	uint32_t start = (uint32_t)memstart&0xFFFFF000;
	uint32_t start_addr = start;
	uint32_t end = ((uint32_t)memend+PAGE_SIZE-1)&0xFFFFF000;
	uint32_t end_addr = end;
	//Add that many pages to our allocated memory
	vmem_size+=(end-start);
	
	start = start>>22;		  //First page_entry in task proc_dir
	end = end>>22;			  //Last page_entry in task proc_dir
	uint32_t page_entries = ((end-start)+2); //Add an extra page so that we have one page for the heap at least
	vmem_size+=page_entries*PAGE_SIZE; //Additional size we need for page dir entries
	
	void* vmem = kalloc(vmem_size);
	memzero(vmem, vmem_size);
	//Create page directory
	page_dir_t* page_dir = page_dir_create(vmem);
	create_kernel_pages(page_dir);
	
	//terminal_writestring("Mapping memory between: ");
	//terminal_writeuint32((uint32_t)memstart);
	//terminal_writeuint32((uint32_t)memend);
	//terminal_writestring("\n");
	//Create page_tables for tasks memory region
	for(uint32_t addr = ((uint32_t)memstart)&0xFFFFF000; addr<=(uint32_t)memend+2*PAGE_SIZE; addr+=PAGE_SIZE)
	{
			uint32_t iv_addr = (uint32_t) addr;
			uint16_t iv_te = iv_addr >> 22;		//Upper 10bits
			uint32_t offset = 1+(iv_te-start); 	//Start is still the index of our first page table entry
			
			map_dir(page_dir,					//Map directory entry
			(void*)addr,						//For current virtual address
			(void*)((vmem+(page_entries+1)*PAGE_SIZE)-start_addr+iv_addr), //To physical address that corresponds to correct kernel virtual address
			vmem+offset*PAGE_SIZE,				//Use vmem+offset*PAGE_SIZE as entry
			PG_User|PG_Present|PG_RW|PG_WriteThrough); //Set correct Page flags
	}
	
	task->ker_mem_start = vmem+(page_entries+1)*PAGE_SIZE;
	task->ker_mem_end = vmem+(page_entries+1)*PAGE_SIZE-start_addr+end_addr;
	
	uint32_t first_tbl = ((uint32_t)vmem)>>22;
	//Map page dir entries in process as well as processes pagedir itself to their corresponding kernel tables
	for(uint32_t tbl = 0; tbl<page_entries+2; tbl++)
	{
			uint16_t ctbl = tbl + first_tbl;
			page_dir_entry_create(page_dir->entries+ctbl, (void*)(KMEM_PG_TABLE_LOC+ctbl*PAGE_SIZE), PG_Present|PG_RW|PG_WriteThrough|PG_Global);
	}
	
	
	return page_dir;
}

task_t* create_task(void (*entry)(), void* memstart, void* memend,uint16_t ss, uint16_t cs, uint16_t ds, int8_t priority)
{
	task_t* new_task = kalloc(sizeof(task_t));
	
	new_task->entry = entry;
	new_task->esp = ((uint32_t)new_task)+PAGE_SIZE;
	
	new_task->level = cs&0x3;
	new_task->priority = 0;
	new_task->priority_mod = priority;
	new_task->time_slice=0;
	new_task->waiting_on=0;
	new_task->pid = pid_counter++;
	new_task->state = TSK_Waiting;
	
	//If we run as kernel we don't need to/should change our page dir
	if(new_task->level)
	{
		page_dir_t* proc_page_dir = create_task_pagedir(memstart, memend+PAGE_SIZE, new_task);
		new_task->cr3 = proc_page_dir;
		new_task->esp = (((uint32_t)new_task->ker_mem_end + PAGE_SIZE - 1)&0xFFFFF000)+PAGE_SIZE;
	}
	else
		new_task->cr3 = 0;
	
	task_context_t* stack = (task_context_t*)(new_task->esp-sizeof(task_context_t));
	memzero((void*)stack, sizeof(stack));
	stack->ss = ss;
	
	//We only want to use memend as an indicator for esp and ebx if we
	//Aren't running in the kernel
	if(new_task->level == 0)
	{
		stack->flags = 0;		
		stack->esp = (uint32_t) stack;
	}
	else
	{
		stack->flags = 512;
		//Create Stack in second to last memory page
		stack->esp = (((uint32_t)memend + PAGE_SIZE - 1)&0xFFFFF000)+PAGE_SIZE-1;
		//Create Heap growing the other way in page after that
		stack->ebx = (((uint32_t)memend + PAGE_SIZE - 1)&0xFFFFF000)+PAGE_SIZE;
	}
	
	stack->eax = new_task->pid;
	stack->cs = cs;
	stack->ds = ds;
	stack->es = ds;
	stack->fs = ds;
	stack->gs = ds;
	stack->esp2 = ((uint32_t) stack) + 4;
	stack->entry = (uint32_t)entry;
	
	new_task->esp = (uint32_t) stack;
	return new_task;
}

void task_print(task_t* task)
{
	terminal_writestring("Task: ");
	terminal_writeuint16(task->pid);
	
	task_context_t* stack = (task_context_t*)(task->esp);
	terminal_writeuint32((uint32_t)stack->entry);
	terminal_writestring("CR3: ");
	terminal_writeuint32((uint32_t)task->cr3);
	terminal_writestring("\n");
	
	terminal_writestring("GS: ");
	terminal_writeuint16(stack->gs);
	terminal_writestring("  FS: ");
	terminal_writeuint16(stack->fs);
	terminal_writestring("  ES: ");
	terminal_writeuint16(stack->es);
	terminal_writestring("  DS: ");
	terminal_writeuint16(stack->ds);
	terminal_writestring("\n");
	
	terminal_writestring("EDI: ");
	terminal_writeuint32(stack->edi);
	terminal_writestring("ESI: ");
	terminal_writeuint32(stack->esi);
	terminal_writestring("EBP: ");
	terminal_writeuint32(stack->ebp);
	terminal_writestring("\n");
	
	terminal_writestring("EBX: ");
	terminal_writeuint32(stack->ebx);
	terminal_writestring("EDX: ");
	terminal_writeuint32(stack->edx);
	terminal_writestring("ECX: ");
	terminal_writeuint32(stack->ecx);
	terminal_writestring("\n");
	
	terminal_writestring("EAX: ");
	terminal_writeuint32(stack->eax);
	terminal_writestring("ENT: ");
	terminal_writeuint32(stack->entry);
	terminal_writestring("ESP: ");
	terminal_writeuint32(stack->esp);
	terminal_writestring("\n");
	
	terminal_writestring("CS:  ");
	terminal_writeuint32(stack->cs);
	terminal_writestring("SS:  ");
	terminal_writeuint32(stack->ss);
	terminal_writestring("FLG: ");
	terminal_writeuint32(stack->flags);
	terminal_writestring("\n");
}