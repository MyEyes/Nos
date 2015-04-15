#include "int.h"

inline void enable_interrupts()
{
	__asm__("sti");
}

inline void disable_interrupts()
{
	__asm__("cli");
}

inline void halt()
{
	__asm__("hlt");
}