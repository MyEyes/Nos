#include "meminfo.h"
#include "../util/terminal.h"

meminfo_page_t* meminfo_store;

void set_meminfo_ptr(meminfo_page_t** firstPage)
{
	meminfo_store = (meminfo_page_t*)(((void*)*firstPage));
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
/*
void add_mem_page(meminfo_page_t* page)
{
	
}

void add_mem(uint32_t base_addr, uint32_t size)
{
	
}
*/

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