#include <unistd.h>

pid_t curr_pid;

pid_t get_current_pid()
{
	return curr_pid;
}