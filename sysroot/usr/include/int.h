#include <stdint.h>
#define call_int(interrupt)	__asm__("int %0":: "N"((interrupt)):"cc", "memory");

void enable_interrupts();
void disable_interrupts();
void halt();