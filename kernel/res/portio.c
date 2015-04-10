#include "portio.h"
inline void outb(uint16_t port, uint8_t val)
{
	__asm__("outb %1, %0": :"dN" (port), "a" (val));
}
void outw(uint16_t port, uint16_t val)
{
	__asm__("outw %1, %0": :"dN" (port), "a" (val));
}
void outl(uint16_t port, uint32_t val)
{
	__asm__("outl %1, %0": :"dN" (port), "a" (val));
}

uint8_t inb(uint16_t port)
{
	uint8_t val;
	__asm__("inb %1, %0" : "=a" (val) : "dN" (port));
	return val;
}
uint16_t inw(uint16_t port)
{
	uint16_t val;
	__asm__("inw %1, %0" : "=a" (val) : "dN" (port));
	return val;
}
uint32_t inl(uint16_t port)
{
	uint32_t val;
	__asm__("inl %1, %0" : "=a" (val) : "dN" (port));
	return val;
}