#include "paging.h"
#include "mem.h"
#include "../util/terminal.h"
#include <stdint.h>
#include "../kernel.h"
#include "meminfo.h"
#include "scheduler.h"
#include "../util/debug.h"

page_dir_t* kernel_page_dir;

page_table_t* page_table_create(void* addr)
{
	uint32_t i_addr = (uint32_t) i_addr;
	i_addr &= 0xFFFFF000;
	memzero(addr, sizeof(page_table_t)); //Clear out memory
	return (page_table_t*)i_addr;
}

page_dir_t* page_dir_create(void* addr)
{
	uint32_t i_addr = (uint32_t) i_addr;
	i_addr &= 0xFFFFF000;
	memzero(addr, sizeof(page_dir_t)); //Clear out memory
	return (page_dir_t*)i_addr;
}

page_dir_entry_t* page_dir_entry_create(page_dir_entry_t* entry, page_table_t* table, page_flags flags)
{
	uint32_t i_table = (uint32_t) table;
	i_table &= 0xFFFFF000;
	i_table |= flags & 0xFFF;
	entry->table = (page_table_t*) i_table;
	return entry;
}

page_table_entry_t* page_table_entry_create(page_table_entry_t* entry, void* physAddr, page_flags flags)
{
	uint32_t i_addr = (uint32_t) physAddr;
	i_addr &= 0xFFFFF000;
	i_addr |= flags & 0xFFF;
	entry->address = (void*) i_addr;
	return entry;
}

void unmap_dir(page_dir_t* dir, void* v_addr)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = iv_addr >> 22;		//Upper 10bits
	uint32_t i_tbl = (uint32_t)dir->entries[iv_te].table;
	if(!(i_tbl&PG_Present))			//If page table doesn't exist, we're already done
		return;
	unmap_tbl(PAGE_ENTRY_TO_PTR(i_tbl), v_addr);
}

void map_dir(page_dir_t* dir, void* v_addr, void* p_addr)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = iv_addr >> 22;		//Upper 10bits
	uint32_t i_tbl = (uint32_t)dir->entries[iv_te].table;
	if(!(i_tbl&PG_Present))			//If page table doesn't exist
	{
		page_dir_entry_create(dir->entries+iv_te, (void*)(KMEM_PG_TABLE_LOC+iv_te*PAGE_SIZE), PG_RW|PG_Present);
		i_tbl = (uint32_t)dir->entries[iv_te].table;
	}
	map_tbl(PAGE_ENTRY_TO_PTR(i_tbl), v_addr, p_addr);
}

void map_tbl(page_table_t* tbl, void* v_addr, void* p_addr)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = (iv_addr >> 12)&0x3FF;		//Lower 10 bits of upper 20bits
	uint32_t i_addr = (uint32_t)tbl->entries[iv_te].address;
	if(i_addr&PG_Present)
	{
		terminal_writestring("Remapping vAddr: ");
		terminal_writeuint32((uint32_t)PAGE_ENTRY_TO_PTR(v_addr));
		terminal_writestring("from: ");
		terminal_writeuint32((uint32_t)PAGE_ENTRY_TO_PTR(i_addr));
		terminal_writestring("to: ");
		terminal_writeuint32((uint32_t)PAGE_ENTRY_TO_PTR(p_addr));
		terminal_writestring("\n");
		flush_tlb_single((uint32_t)v_addr);
	}
	page_table_entry_create((tbl->entries+iv_te), p_addr, PG_Present | PG_RW);
}

void unmap_tbl(page_table_t* tbl, void* v_addr)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = (iv_addr >> 12)&0x3FF;		//Lower 10 bits of upper 20bits
	uint32_t i_addr = (uint32_t)tbl->entries[iv_te].address;
	if(i_addr&PG_Present)
	{
		tbl->entries[iv_te].address=(void*)0;
		flush_tlb_single((uint32_t)v_addr);
	}
}

inline void flush_tlb_single(uint32_t addr)
{
   asm volatile("invlpg (%0)" ::"r" (addr) : "memory");
}

void enable_paging(page_dir_t* dir)
{
	__asm__("movl %0, %%eax\n\t"
			"movl %%eax, %%cr3\n\t"
			"movl %%cr0, %%eax\n\t"
			"orl $0x80000000, %%eax\n\t"
			"movl %%eax, %%cr0\n\t": : "dN" (dir): "eax", "memory");
}

void paging_handle_pagefault(void* vAddr, uint32_t err_code)
{
	//terminal_writestring("Page Fault\n");
	//err_code bits
	//0			1		2		3			4
	//present	write	user	res_write	instruction
	if(!(err_code&1)) //If the fault was caused because a page wasn't present
	{
		void* pAddr = pmem_get_free_page();
		map_dir(kernel_page_dir,vAddr,pAddr);
		int16_t owner = PMEM_KERNEL_OWNER;
		if((err_code&0x4))
			owner = get_current_pid();
		pmem_mod((uint32_t)pAddr, owner, PMEM_MAPPED);
	}
	else
	{
		if((err_code&0x4))
		{
			terminal_writestring("Access violation, killing process\n");
			bochs_break();
			//Reset stack to where it should be for a scheduler call
			/*uint32_t esp = KMEM_SYSCALL_STACK_LOC;
			__asm__ (	"mov %0, %%eax\r\n"
						"sub $5*4, %%eax\r\n"
						"mov %%eax, %%esp": : "m"(esp) :"memory", "%eax");
						*/
			schedule_kill();
		}
	}
}

void init_kernel_paging()
{
	//Identity Map kernel memory
	kernel_page_dir = page_dir_create(KMEM_PG_DIR_LOC);
	page_dir_create((void*)PAGE_SIZE);
	memzero((void*)KMEM_PG_TABLE_LOC, KMEM_PG_TABLE_SIZE);
	//Go through meminfo to find avaiable RAM regions
	for(uint32_t index = 0; index<100; index++)
	{
		meminfo_page_t* page;
		if((page = get_free_meminfo_page(index)))
		{
			print_meminfo_page(page);
			for(uint32_t addr = page->base_addr;
			addr+PAGE_SIZE <= page->base_addr+page->reg_length && addr+PAGE_SIZE<=KMEM_KERNEL_LIMIT;
			addr+=PAGE_SIZE)
			{
				map_dir(kernel_page_dir, (void*)addr, (void*)addr);
			}
		}
		else
			break;
	}
	//Tell meminfo that we have taken up some memory for the kernels exclusive use
	pmem_mod_range(0, KMEM_KERNEL_LIMIT, PMEM_KERNEL_OWNER, PMEM_KERN|PMEM_MAPPED);
	enable_paging(kernel_page_dir);
}