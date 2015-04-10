#include "mem.h"

void memcpy(char* trg, char* src, size_t num)
{
	for(size_t cnt = 0; cnt<num; cnt++)
	{
		trg[cnt] = src[cnt];
	}
}

void memzero(char* trg, size_t num)
{
	for(size_t cnt = 0; cnt<num; cnt++)
	{
		trg[cnt] = 0;
	}
}