#include "portio.h"
inline void outb(uint16_t port, uint8_t val)
{
	__asm__ __volatile__("outb %1, %0": : "dN" (port), "a" (val));
}
void outw(uint16_t port, uint16_t val)
{
	__asm__ __volatile__("outw %1, %0": : "dN" (port), "a" (val));
}
void outl(uint16_t port, uint32_t val)
{
	__asm__ __volatile__("outl %1, %0": : "dN" (port), "a" (val));
}


void wait_io()
{
	__asm__ __volatile__("outb %%al, $0x80": : "a"(0));
}

uint8_t inb(uint16_t port)
{
	uint8_t val;
	__asm__ __volatile__ ("inb %1, %0" : "=a" (val) : "dN" (port));
	return val;
}
uint16_t inw(uint16_t port)
{
	uint16_t val;
	__asm__ __volatile__("inw %1, %0" : "=a" (val) : "dN" (port));
	return val;
}
uint32_t inl(uint16_t port)
{
	uint32_t val;
	__asm__ __volatile__("inl %1, %0" : "=a" (val) : "dN" (port));
	return val;
}