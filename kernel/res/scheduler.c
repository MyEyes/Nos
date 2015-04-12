#include "scheduler.h"
#include "../util/idt.h"
#include "../util/task.h"
#include "../util/debug.h"
#include "../util/terminal.h"
#include "../util/clock.h"

//Jump array
//Empty entry points to next filled one
task_t* tasks[SCHEDULER_MAX_TASKS];

task_t* old_task;
task_t* curr_task;
task_t* next_task;

uint8_t schedule_switch_flag;

extern void (*schedule_handler)();

__attribute__((noreturn)) void scheduler_spawn(task_t* task)
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
		curr_task = task;
		call_task(task);
	}
}

void init_scheduler()
{
	set_idt_desc(IRQ_OFFSET+0x00, (uint32_t)&schedule_handler, 0, IntGate32, 0x08);
	//Initialize jump array
	tasks[0] = (void*)SCHEDULER_MAX_TASKS;
}

void schd_task_add(task_t* task)
{
	for(uint16_t index = 0; index<SCHEDULER_MAX_TASKS; index++)
	{
		if((uint32_t)tasks[index] <= SCHEDULER_MAX_TASKS )
		{
			//Insert at first free location
			//Set next slot to correct offset
			uint32_t offset = (uint32_t)tasks[index];
			offset--;
			if(offset)
				tasks[index+1] = (void*)offset;
			tasks[index] = task;
			break;
		}
	}
}

void schd_task_del(task_t* task)
{
	for(uint32_t index = 0; index<SCHEDULER_MAX_TASKS; index++)
	{
		if(!tasks[index])	//If we hit a 0 we are done
			return;
		if((uint32_t)tasks[index]<=SCHEDULER_MAX_TASKS)
		{
			index = (uint32_t) tasks[index]-1; //Subtract one because the next loop will increment again
		}
		else if(tasks[index] == task)
		{
			tasks[index] = 0;
			for(uint32_t findnext=0; findnext<SCHEDULER_MAX_TASKS-index; findnext++)
				if((uint32_t)tasks[index]>SCHEDULER_MAX_TASKS)
				{
					tasks[index] = (void*)findnext;
					break;
				}
			return;
		}
	}
}

/*
Moved to ASM
void yield_control()
{
	__asm__("add $0x14, %%esp": : :"memory"); 
	//Workaround because of c calling convention
	//We don't return from here so we clean the stack
	//by hand
	old_task = curr_task;
	curr_task = next_task;
	
	old_task->time_slice = 0;
	old_task->state = TSK_Waiting;
	old_task->priority = 0;
	
	next_task->state = TSK_Running;
	next_task->time_slice = next_task->priority;
	
	switch_task(old_task, curr_task);
}
*/

void schedule()
{
	schedule_switch_flag=0;
	uint64_t time_diff = clock_time_diff();
	
	if(curr_task)
		curr_task->time_slice -= time_diff;
	
	if(!curr_task || curr_task->time_slice<0)
	{
		schedule_switch_flag = 1;					//We want to switch if our current process is done for now
		curr_task->priority=0;
	}
	
	uint64_t current_prio = 0;
	
	for(uint16_t index=1; index<SCHEDULER_MAX_TASKS; index++)
	{
		if((uint32_t)tasks[index]<=SCHEDULER_MAX_TASKS)	//Tells us to jump forward
		{
			//Go to next entry
			if(tasks[index])
				index = (uint32_t)tasks[index]-1;
			else
			{
				if(next_task == 0)
					schedule_switch_flag=0;
				else
				{
					next_task->time_slice = next_task->priority*100;
				}
				return;
			}
		}
		else if(tasks[index]->state == TSK_Waiting)	//If we don't jump it's a process
		{
			//Increase priority of waiting processes depending on priority_mod
			if(tasks[index]->priority_mod >= 0)
				tasks[index]->priority += time_diff << tasks[index]->priority_mod;
			else
				tasks[index]->priority += time_diff >> -tasks[index]->priority_mod;
			
			//Find highest priority process
			if(tasks[index]->priority > current_prio)
			{
				next_task = tasks[index];
				current_prio = tasks[index]->priority;
			}
		}
	}
}