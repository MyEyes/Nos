#include <string.h>

size_t strlen(const char* str)
{
	size_t ret = 0;
	while ( str[ret] != 0 )
		ret++;
	return ret;
}

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
	return memset(trg, 0, num);
}

void* memset(void* trg, int val, size_t num)
{
	char* dst8 = (char*) trg;
	while(num--)
	{
		*dst8++ = val;
	}
	return trg;
}