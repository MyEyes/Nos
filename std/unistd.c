#include <unistd.h>
#include <idt.h>
#include <int.h>

pid_t curr_pid;

pid_t get_current_pid()
{
	return curr_pid;
}

void sleep()
{
	__asm__ ("mov $0, %%eax":::"memory");
	call_int(PROC_YIELD);
}