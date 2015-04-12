#include "scheduler.h"
#include "../util/idt.h"
#include "../util/task.h"
#include "../util/debug.h"
#include "../util/terminal.h"

task_t* tasks[SCHEDULER_MAX_TASKS];
task_t* old_task;
task_t* curr_task;
task_t* next_task;

uint8_t schedule_switch_flag;

extern void (*schedule_handler)();

void scheduler_spawn(task_t* task)
{
	schd_task_add(task);
	
	if(curr_task)
	{
		old_task=curr_task;
		curr_task = task;
		switch_task(old_task, curr_task);
	}
	else
	{
		curr_task=task;
		call_task(task);
	}
}

void init_scheduler()
{
	set_idt_desc(IRQ_OFFSET+0x00, (uint32_t)&schedule_handler, 0, TrapGate32, 0x08);
}

void schd_task_add(task_t* task)
{
	for(uint16_t index = 0; index<SCHEDULER_MAX_TASKS; index++)
	{
		if(tasks[index] == 0)
		{
			tasks[index] = task;
			break;
		}
	}
	
}

void schd_task_del(task_t* task)
{
	for(uint16_t index = 0; index<SCHEDULER_MAX_TASKS; index++)
	{
		if(tasks[index] == task)
		{
			tasks[index] = 0;
			return;
		}
	}
}

void yield_control()
{
	//__asm__("pop %%eax": : :"memory"); 
	//Pop return address from stack since
	//we've already set up all the return
	//stuff for an iret
	switch_task(curr_task, next_task);
}

void schedule()
{
	schedule_switch_flag=0;
	terminal_writeuint32(tasks[0]);
	terminal_writeuint32(tasks[1]);
	terminal_writeuint32(tasks[2]);
	
	for(uint16_t index=1; index<SCHEDULER_MAX_TASKS; index++)
	{
		if(tasks[index] && tasks[index]!=curr_task)
		{
			schedule_switch_flag = 1;
			next_task = tasks[index];
			bochs_break();
			__asm__("mov %eax, %eax");
			return;
		}
	}
}