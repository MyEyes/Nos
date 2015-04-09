#include "int.h"

void enable_interrupts()
{
	__asm__("sti");
}

void disable_interrupts()
{
	__asm__("cli");
}

void halt()
{
	__asm__("hlt");
}