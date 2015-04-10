#include "debug.h"
void bochs_break()
{
	__asm__  __volatile__ ("xchg %bx, %bx");
}