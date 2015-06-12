#include <stdio.h>
#include <ipc/ipc.h>
#include <string.h>

int print(char* str)
{
	return send_to_port(1, 0, str, strlen(str));
}