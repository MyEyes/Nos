#include <stdbool.h> /* C doesn't have booleans by default. */
#include <stddef.h>
#include <stdint.h>
#include "util/terminal.h"
#include "res/meminfo.h"
#include "util/int.h"
#include "res/gdt.h"
#include "util/idt.h"

void kernel_main()
{
	terminal_initialize();
	/* Since there is no support for newlines in terminal_putchar yet, \n will
	   produce some VGA specific character instead. This is normal. */
	terminal_writestring("Hello, kernel World!\n\n");
	set_meminfo_ptr((meminfo_page_t**)(0x14000));
	print_meminfo();
	
	terminal_writestring("\n");
	setup_gdt(*(gdt_info_t**)(0x14010));
	print_gdt_info();
	setup_idt();
	load_idt();
	__asm__("int $0x80");
	enable_interrupts();
	halt();
}

void kernel_panic()
{
	terminal_initialize();
	terminal_writestring("Kernel Panic\nEmergency halt\n");
	disable_interrupts();
	halt();
	while(1); //Infinite loop if the halt somehow breaks
}

void interrupt_happened()
{
	terminal_writestring("There was an interrupt!\n");
}