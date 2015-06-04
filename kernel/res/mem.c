#include <stddef.h>

void* memcpy(void* trg, const void* src, size_t num)
{
	char* dst8 = (char*) trg;
	char* src8 = (char*) src;
	while(num--)
	{
		*dst8++ = *src8++;
	}
	return trg;
}

void* memzero(void* trg, size_t num)
{
	char* dst8 = (char*) trg;
	while(num--)
	{
		*dst8++ = 0;
	}
	return trg;
}