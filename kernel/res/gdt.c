#include "gdt.h"
#include "../util/terminal.h"

gdt_info_t* gdt_info = (gdt_info_t*)0;

void setup_gdt(gdt_info_t* ptr)
{
	gdt_info = ptr;
}

void print_gdt_info()
{
	terminal_writestring("Current GDT Information: \n");
	terminal_writestring("GDT Info: ");
	terminal_writeuint32((uint32_t)gdt_info);
	terminal_writestring("\n");
	terminal_writestring("GDT Start: ");
	terminal_writeuint32((uint32_t)gdt_info->entry_ptr);
	terminal_writestring("\n");
	terminal_writestring("GDT Size: ");
	terminal_writeuint32((uint32_t)gdt_info->size);
	terminal_writestring("\n\n");
	terminal_writestring("GDT Contents: \n");
	for(size_t offset=0; offset*8<gdt_info->size; offset++)
	{
		gdt_entry_raw_t curr = gdt_get_raw_entry(offset);
		gdt_entry_t currE = raw_to_gdt_entry(curr);
		gdt_print_entry(currE);
	}
}

gdt_entry_t raw_to_gdt_entry(gdt_entry_raw_t entry)
{
	gdt_entry_t result;
	result.size = entry.Fl & 0x0F; //Lowest 4 bits of TT
	result.size <<= 8; //Shift 8 bits
	result.size |= entry.l1 & 0xFF;
	result.size <<= 8;
	result.size |= entry.l0 & 0xFF;
	//Done with size
	result.base_addr = entry.b3 & 0xFF;
	result.base_addr <<= 8;
	result.base_addr |= entry.b2 & 0xFF;
	result.base_addr <<= 8;
	result.base_addr |= entry.b1 & 0xFF;
	result.base_addr <<= 8;
	result.base_addr |= entry.b0 & 0xFF;
	//Done with addr
	result.flags = entry.Fl & 0xF0;
	result.type = entry.TT;
	//Assign flags and we're done
	return result;
}

gdt_entry_raw_t gdt_get_raw_entry(uint32_t index)
{
	gdt_entry_raw_t dummy;
	if(index*sizeof(gdt_entry_raw_t)>gdt_info->size)
	{
		terminal_writestring("gdt_entry index out of range\n");
		return dummy;
	}
	return gdt_info->entry_ptr[index];
}

void gdt_print_entry(gdt_entry_t entry)
{
	terminal_writeuint64(entry.base_addr);
	terminal_writeuint32(entry.size);
	terminal_writeuint8(entry.type);
	terminal_writeuint8(entry.flags);
	terminal_writestring("\n");
}

gdt_entry_raw_t gdt_entry_to_raw(gdt_entry_t entry)
{
	gdt_entry_raw_t result;
	result.b0 = entry.base_addr & 0xFF;
	result.b1 = (entry.base_addr>>8) & 0xFF;
	result.b2 = (entry.base_addr>>16) & 0xFF;
	result.b3 = (entry.base_addr>>24) & 0xFF;
	result.l0 = (entry.size) & 0xFF;
	result.l1 = (entry.size>>8) & 0xFF;
	result.Fl = entry.flags & 0xF0;
	result.Fl |= (entry.size>>16) & 0x0F;
	result.TT = entry.type;
	return result;
}