#include <stdlib.h>
#include <ipc/ipc.h>

void std_init()
{
	init_ipc();
}

extern void (*_fini)();

void abort()
{
	exit(-1);
}