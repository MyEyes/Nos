#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <string.h>
#include <drv_tab.h>
#include <paging.h>
#include <stdlib.h>
#include <format.h>
#include <stdarg.h>

int print(const char* str)
{
	int res = send_to_port(IPC_PORT_TERMINAL, IPC_PORT_NONE, str, strlen(str)+1);
	yield_control_to_port(IPC_PORT_TERMINAL);
	return res;
}	

int printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	size_t len = _format_length(format, &ap);
	va_end(ap);
	char* buf = (char*)malloc(len+1);
	va_start(ap, format);
	_sprintf(buf, format, &ap);
	int res = puts(buf);
	free(buf);
	va_end(ap);
	return res;
}

int puts(const char *s)
{
	return print(s);
}

char getc()
{
	int portnum = get_free_port();
	init_port(portnum, 128);
	open_port(portnum);
	char cmd[1];
	cmd[0] = 1;
	ipc_msg_hdr_t curr_msg;
	send_to_port(IPC_PORT_KEYBOARD, portnum, (char*)cmd, 1);
	yield_control_to_port(IPC_PORT_KEYBOARD);
	while(get_ipc_message(portnum, &curr_msg, (char*)cmd, 1)<0);
	free_port(portnum);
	return cmd[0];
}