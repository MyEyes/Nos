#include "terminal_drv.h"
#include <ipc/ipc.h>
#include <int.h>
#include <task.h>
#include <kalloc.h>
#include <terminal.h>
#include <scheduler.h>
#include <gdt.h>
#include <debug.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <drv_tab.h>

task_t* terminal_task;

ipc_port_t* term_port;
ipc_msg_hdr_t curr_msg;
char* buffer;

void start_terminal_drv()
{
	buffer = kalloc(sizeof(char)*4000);
	terminal_task = create_task(terminal_drv_run, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	schd_task_add(terminal_task);
}


void terminal_drv_run()
{
	term_port = init_port(IPC_PORT_TERMINAL, 4000);
	open_port(IPC_PORT_TERMINAL);
	enable_interrupts();
	//Infinite loop waiting for messages
	while(1)
	{
		//If we got a message we print it
		if(!get_ipc_message(IPC_PORT_TERMINAL, &curr_msg, buffer, 4000))
		{
			terminal_writestring(buffer);
		}
		else
		{
			sleep();
		}
	}
}

int print(const char* text)
{
	send_to_port(IPC_PORT_TERMINAL, -1, text, strlen(text)+1);
	return 0;
}