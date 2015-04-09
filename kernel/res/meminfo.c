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

void print_acpi_ext(uint32_t val)
{
	terminal_writeuint32(val);
	terminal_writestring("\n");
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
		size_t* currval= (size_t*) currentPage;
		terminal_writeuint64(currval[0]);
		terminal_writeuint64(currval[2]);
		print_mem_type(currentPage->reg_type);
		print_acpi_ext(currentPage->acpi_ext);
		currentPage+=1;
	}
}