#include <stddef.h>

size_t strlen(const char* str)
{
	size_t ret = 0;
	while ( str[ret] != 0 )
		ret++;
	return ret;
}

void ctohs(char in, char* buf)
{
	const char* hexVals = "0123456789ABCDEF";
	size_t sz = sizeof(char);
	for(size_t x=0; x<sz*2; x++)
	{
		size_t val = in & 0xF;
		buf[sz*2-x-1] = hexVals[val];
		in = in>>4;
	}
}