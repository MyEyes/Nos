#include <stdint.h>
#include <string.h>
#include <paging.h>
#include <terminal.h>
#include <kernel.h>
#include <meminfo.h>
#include <scheduler.h>
#include <debug.h>

page_dir_t* kernel_page_dir;

page_table_t* page_table_create(void* addr)
{
	uint32_t i_addr = (uint32_t) addr;
	i_addr &= 0xFFFFF000;
	memzero((void*)i_addr, sizeof(page_table_t)); //Clear out memory
	return (page_table_t*)i_addr;
}

page_dir_t* page_dir_create(void* addr)
{
	uint32_t i_addr = (uint32_t) addr;
	i_addr &= 0xFFFFF000;
	memzero((void*)i_addr, sizeof(page_dir_t)); //Clear out memory
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

void kernel_map_page(void* v_addr, void* p_addr, page_flags flags)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = iv_addr >> 22;		//Upper 10bits
	void *tbl_loc = (void*)(KMEM_PG_TABLE_LOC+iv_te*PAGE_SIZE);
	map_dir(kernel_page_dir, v_addr, p_addr, tbl_loc, flags);
}

void map_dir(page_dir_t* dir, void* v_addr, void* p_addr, void* tbl_loc, page_flags flags)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = iv_addr >> 22;		//Upper 10bits
	uint32_t i_tbl = (uint32_t)dir->entries[iv_te].table;
	if(!(i_tbl&PG_Present))			//If page table doesn't exist
	{
		//Set User so we can mark wether something is a user page on mem page level
		//Otherwise the priv check fails already at the directory
		page_dir_entry_create(dir->entries+iv_te, tbl_loc, flags|PG_User);
		i_tbl = (uint32_t)dir->entries[iv_te].table;
	}
	map_tbl(PAGE_ENTRY_TO_PTR(i_tbl), v_addr, p_addr, flags);
}

void map_tbl(page_table_t* tbl, void* v_addr, void* p_addr, page_flags flags)
{
	uint32_t iv_addr = (uint32_t) v_addr;
	uint16_t iv_te = (iv_addr >> 12)&0x3FF;		//Lower 10 bits of upper 20bits
	uint32_t i_addr = (uint32_t)tbl->entries[iv_te].address;
	if(i_addr&PG_Present && 0)
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
	page_table_entry_create((tbl->entries+iv_te), p_addr, flags);
	if(flags&PG_Global)
		flush_tlb_single((uint32_t) v_addr);
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
			"movl %%eax, %%cr3\n\t"		//Mov dir into cr3 register
			"movl %%cr0, %%eax\n\t"		//Set paging flag in cr0
			"orl $0x80000000, %%eax\n\t"
			"movl %%eax, %%cr0\n\t"
			"movl %%cr4, %%eax\n\t"		//Set global paging flag in cr4
			"orl $0x00000080, %%eax\n\t"
			"movl %%eax, %%cr4": : "dN" (dir): "eax", "memory");
}

void paging_handle_pagefault(void* vAddr, uint32_t err_code)
{
	//err_code bits
	//0			1		2		3			4
	//present	write	user	res_write	instruction
	if(!(err_code&1)) //If the fault was caused because a page wasn't present
	{
		void* pAddr = pmem_get_free_page();
		kernel_map_page(vAddr,pAddr, KERNEL_PAGE_FLAGS);
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

void create_kernel_pages(page_dir_t* page_dir)
{
	for(uint32_t tbl = 0;
	tbl<(KMEM_KERNEL_LIMIT>>22);
	tbl++)
	{
		page_dir_entry_create(page_dir->entries+tbl, (void*)(KMEM_PG_TABLE_LOC+tbl*PAGE_SIZE), KERNEL_PAGE_FLAGS|PG_Global);
	}
}


void init_kernel_paging()
{
	//Identity Map kernel memory
	kernel_page_dir = page_dir_create(KMEM_PG_DIR_LOC);
	page_dir_create((void*)PAGE_SIZE);
	memzero((void*)KMEM_PG_TABLE_LOC, KMEM_PG_TABLE_SIZE);
	
	for(uint32_t addr = 0;
	addr+PAGE_SIZE<=KMEM_KERNEL_LIMIT;
	addr+=PAGE_SIZE)
	{
		uint16_t i_te = addr >> 22;		//Upper 10bits
		map_dir(kernel_page_dir, (void*)addr, (void*)addr, (void*)(KMEM_PG_TABLE_LOC+i_te*PAGE_SIZE), KERNEL_PAGE_FLAGS|PG_Global);
	}

	//Tell meminfo that we have taken up some memory for the kernels exclusive use
	pmem_mod_range(0, KMEM_KERNEL_LIMIT, PMEM_KERNEL_OWNER, PMEM_KERN|PMEM_MAPPED);
	enable_paging(kernel_page_dir);
	bochs_break();
}