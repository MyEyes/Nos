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

task_t* terminal_task;

ipc_port_t* term_port;
uint32_t term_port_id = 1;
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
	term_port = init_port(term_port_id, 4000);
	open_port(term_port_id);
	enable_interrupts();
	terminal_writestring("(Hello, I'm your terminal driver)\n");
	//Infinite loop waiting for messages
	while(1)
	{
		//If we got a message we print it
		terminal_writestring("(Checking for messages)\n");
		bochs_break();
		if(!get_ipc_message(term_port_id, &curr_msg, buffer, 4000))
		{
			//terminal_writestring("Got a message\n");
			terminal_writestring(buffer);
		}
		else
		{
			terminal_writestring("(Going to sleep)\n");
			sleep();
		}//Ideally sleep here	;
	}
}