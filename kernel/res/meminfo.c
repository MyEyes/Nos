#include "meminfo.h"
#include "../util/terminal.h"
#include "../kernel.h"
#include "paging.h"
#include "mem.h"
#include "../util/debug.h"

meminfo_page_t* meminfo_store;
pmem_dir_t* pmem_dir = (pmem_dir_t*) KMEM_PMEM_DIR_LOC;

void set_meminfo_ptr(meminfo_page_t** firstPage)
{
	meminfo_store = (meminfo_page_t*)(((void*)*firstPage));
	memzero((void*)KMEM_PMEM_DIR_LOC, KMEM_PMEM_DIR_SIZE);
	pmem_map();
}

void print_mem_type(uint32_t val)
{
	switch(val)
	{
		case 1: terminal_writestring("Usable   "); break;
		case 2: terminal_writestring("Reserved "); break;
		case 3: terminal_writestring("ACPI Rec "); break;
		case 4: terminal_writestring("ACPI NVS "); break;
		case 5: terminal_writestring("Bad Mem  "); break;
		default: terminal_writestring("         "); break;
	}
}


void pmem_add_mem_page(meminfo_page_t* page)
{
	pmem_entry_flags flags = 0;
	if(page->reg_type < 3) //Usable types
		flags = PMEM_PRESENT;
	if(page->reg_type == 2)
		flags |= PMEM_RESERVED;
	//Don't set owner info because we are just taking index with this
	pmem_mod_range((uint32_t)page->base_addr, (uint32_t)page->reg_length, 0, flags);
}

void pmem_mod_range(uint32_t base_addr, uint32_t size, uint16_t owner, pmem_entry_flags flags)
{
	for(uint32_t addr=base_addr; addr<base_addr+size; addr+=PAGE_SIZE)
	{
		pmem_mod(addr, owner, flags);
	}
}

void pmem_mod(uint32_t addr, uint16_t owner, pmem_entry_flags flags)
{
		uint16_t lindex = (addr>>12) & 0x3FF;	//We just want 10 bits
		uint16_t hindex = (addr>>22) & 0x3FF;	//Top 10 bits
		pmem_dir_entry_t* dir_entry = pmem_dir->entries+hindex;
		if(!PMEM_ADDR(dir_entry->table))
		{
			if(owner==0)						//Don't create tables unless we are taking index
				pmem_create_table(addr);
			else
				return;
		}
		pmem_table_entry_t* tbl_entry = PMEM_ADDR(dir_entry->table)->entries+lindex;
		uint16_t tbl_info = PMEM_INFO(dir_entry->table);
		//If owner is 0 we are initializing
		//Otherwise we can only modify an existing page
		uint32_t cflags = tbl_entry->flags;
		if(owner == 0 || ((cflags&PMEM_PRESENT)))
		{
			tbl_entry->owner = owner;
			//Unless we are taking index we don't want to change the last 2 bits
			//They tell us if the memory exists and is up for use
			if(owner==0)
			{
				if((flags&PMEM_PRESENT) && !(flags&PMEM_RESERVED))
					tbl_info++;
				tbl_entry->flags = flags;
			}
			else
			{
				//We can only really change wether this memory is avaiable
				//if it exists and isn't reserved
				if(!(cflags&PMEM_RESERVED))
				{
					//Add or subtract depending on wether the page is mapped now
					//If we are setting to mapped now but it wasn't before we decrement
					//the tables free counter
					if((flags&PMEM_MAPPED) && !(cflags&PMEM_MAPPED)) 
						tbl_info--;
					//If we are unmapping, we increment the free counter
					else if(!(flags&PMEM_MAPPED) && (cflags&PMEM_MAPPED))
						tbl_info++;
				}
				tbl_entry->flags = (tbl_entry->flags&0x03) | (flags&0xFC);
			}
			
		}
		dir_entry->table = PMEM_IADDR(tbl_entry, tbl_info);
}

void pmem_create_table(uint32_t addr)
{
		uint16_t hindex = (addr>>22) & 0x3FF;	//Top 10 bits
		pmem_dir_entry_t* dir_entry = pmem_dir->entries+hindex;
		//address and 1024 free entries
		dir_entry->table = (pmem_table_t*)PMEM_IADDR((KMEM_PMEM_TABLE_LOC+sizeof(pmem_table_t)*(hindex)), 0);
		memzero((void*)PMEM_ADDR(dir_entry->table), sizeof(pmem_table_t));
}

void* pmem_get_free_page()
{
	for(uint16_t hindex=0; hindex<1024; hindex++)
	{
		pmem_dir_entry_t* dir_entry = pmem_dir->entries+hindex;
		uint16_t tbl_info = PMEM_INFO(dir_entry->table);
		if(tbl_info)
		{
			terminal_writestring("Free hindex ");
			terminal_writeuint16(hindex);
			terminal_writeuint16(tbl_info);
			pmem_table_t* tbl = PMEM_ADDR(dir_entry->table);
			for(uint16_t lindex=0; lindex<1024; lindex++)
			{
				pmem_table_entry_t* entry = tbl->entries+lindex;
				if((entry->flags&PMEM_PRESENT) && !(entry->flags&(PMEM_MAPPED|PMEM_RESERVED)))
					return (void*)(((uint32_t)hindex<<22)+((uint32_t)lindex<<12));
			}
		}
	}
	return (void*)0;
}

void pmem_print_info()
{
	uint32_t start= 0;
	pmem_entry_flags flags = 0;
	
	for(uint32_t x=0; x<((uint32_t)1<<30); x+=PAGE_SIZE)
	{
		uint16_t lindex = (x>>12) & 0x3FF;	//We just want 10 bits
		uint16_t hindex = (x>>22) & 0x3FF;	//Top 10 bits
		pmem_dir_entry_t* dir_entry = pmem_dir->entries+hindex;
		uint16_t print = 0;
		if(dir_entry->table)
		{
			pmem_table_entry_t* table_entry = PMEM_ADDR(dir_entry->table)->entries+lindex;
			if(table_entry->flags)
			{
				if(flags == 0)
				{
					flags = table_entry->flags;
					start = x;
				}
				else if(flags != table_entry->flags)
					print=1;
			}
			else
				print=1;
		}
		else
			print=1;
		if(print && flags)
		{
			terminal_writeuint32(start);
			terminal_writestring("-> ");
			terminal_writeuint32(x-1);
			terminal_writeuint32(flags);
			terminal_writestring("\n");
			flags = 0;
			x-=PAGE_SIZE;	//Set page back by one so we visit the page that canceled again
		}
	}
}

void print_acpi_ext(uint32_t val)
{
	terminal_writeuint32(val);
	terminal_writestring("\n");
}

meminfo_page_t* get_free_meminfo_page(uint8_t index)
{
	if(meminfo_store==0)
		return 0;
	meminfo_page_t* currentPage = meminfo_store;
	while(currentPage->reg_length>0)
	{		
		if(currentPage->reg_type == 1)
		{
			if(index--==0)
				return currentPage;
		}
		currentPage+=1;
	}
	return 0;
}

void pmem_map()
{
	if(meminfo_store==0)
		return ;
	meminfo_page_t* currentPage = meminfo_store;
	while(currentPage->reg_length>0)
	{		
		pmem_add_mem_page(currentPage);
		currentPage+=1;
	}
}

void print_meminfo_page(meminfo_page_t* page)
{
		size_t* currval= (size_t*) page;
		terminal_writeuint64(currval[0]);
		terminal_writeuint64(currval[2]);
		print_mem_type(page->reg_type);
		print_acpi_ext(page->acpi_ext);	
}

void print_meminfo()
{
	if(meminfo_store==0)
		return;
	meminfo_page_t* currentPage = meminfo_store;
	terminal_writestring("--------------------Memory  Info-------------------\n");
	terminal_writestring("Base Addr        Segment Length   Type     ACPI_EXT\n");
	while(currentPage->reg_length>0)
	{	
		print_meminfo_page(currentPage);
		currentPage+=1;
	}
}