#include <scheduler.h>
#include <int.h>
#include <idt.h>
#include <task.h>
#include <debug.h>
#include <terminal.h>
#include <clock.h>
#include <kernel.h>
#include <string.h>
#include <sys/types.h>

//Jump array
//Empty entry points to next filled one
task_t* tasks[SCHEDULER_MAX_TASKS];

task_t* old_task;
task_t* curr_task = 0;
task_t* next_task;

uint8_t schedule_switch_flag;

extern void (*schedule_handler)();
extern void (*yield_handler)();
extern void (*exit_handler)();

void scheduler_spawn(task_t* task)
{
	schd_task_add(task);
	next_task=task;
	call_int(PROC_START);
}

void sleep()
{
	//Set task to sleeping
	//And invoke scheduler interrupt
	curr_task->state = TSK_Sleeping;
	if(curr_task->lender_task && curr_task->lender_task->state== TSK_Waiting)
	{
		next_task = curr_task->lender_task;
		next_task->time_slice = curr_task->time_slice;
		curr_task->lender_task=(task_t*)0;
		curr_task->time_slice = -1;
		call_int(PROC_START);
	}
	else
	{
		curr_task->time_slice = -1;
		call_int(TIMER); //The scheduler hooks the timer interrupts
	}
}

void yield_to()
{
	task_context_t* stack = (task_context_t*)(curr_task->esp);
	int pid = stack->eax;
	//If we call yield with a target pid of 0 we just want to pass on control, so we sleep and schedule
	if(pid==0 || pid>SCHEDULER_MAX_TASKS)
	{
		curr_task->waiting_on = (lock_t*)pid;
		curr_task->state = TSK_Sleeping;
		//terminal_writestring("Making process sleep\n");
		schedule();
		return;
	}
	task_t* tsk = get_task(pid);
	if(tsk)
	{
		//terminal_writestring("yielded control from ");
		task_t* ct = get_current_task();
		//terminal_writeuint32(ct->pid);
		tsk->lender_task = ct;
		tsk->state = TSK_Waiting;
		tsk->time_slice = ct->time_slice;
		curr_task = tsk;
		//terminal_writestring("to ");
		//terminal_writeuint32(curr_task->pid);
		//terminal_writestring("\n");
	}
}

void signal(pid_t pid)
{
	task_t* tsk = get_task(pid);
	if(tsk->state == TSK_Sleeping)
		tsk->state = TSK_Waiting;
}

task_t* get_task(pid_t pid)
{
	for(uint16_t index = 0; index<SCHEDULER_MAX_TASKS; index++)
	{
		if((uint32_t)tasks[index]<=SCHEDULER_MAX_TASKS)
		{
			index+=(uint32_t)tasks[index]-1;
		}
		else if(tasks[index]->pid == pid)
		{
			return tasks[index];
		}
	}
	return 0;
}

void init_scheduler()
{
	terminal_writestring("Initializing scheduler\n");
	memzero(tasks, sizeof(task_t*)*SCHEDULER_MAX_TASKS);
	
	//Initialize jump array
	tasks[0] = (void*)SCHEDULER_MAX_TASKS;
	curr_task = 0;
	next_task = 0;
	
	set_idt_desc(IRQ_OFFSET+0x00, (uint32_t)&schedule_handler, 0, IntGate32, 0x08);
	set_idt_desc(PROC_EXIT, (uint32_t)&exit_handler, 3, IntGate32, 0x08);
	set_idt_desc(PROC_YIELD, (uint32_t)&yield_handler, 3, IntGate32, 0x08);
	
}

void print_current_task()
{
	task_print(curr_task);
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

pid_t get_current_pid()
{
	if(curr_task)
		return curr_task->pid;
	else
		return (uint16_t)-1;
}

task_t* get_current_task()
{
	return curr_task;
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

void schedule_kill()
{
	curr_task->state = TSK_Terminated;
	terminal_writestring("Killed task: \n");
	task_print(curr_task);
	schd_task_del(curr_task);
	
	schedule();
	
	curr_task = next_task;
	terminal_writestring("Resuming\n");
	bochs_break();
	resume_task();
}

void schedule_exit()
{
	curr_task->state = TSK_Exited;
	curr_task->time_slice = 0;
	task_context_t* stack = (task_context_t*)(curr_task->esp);
	int retval = stack->eax;
	terminal_writestring("Task exited ");
	terminal_writeuint32(retval);
	terminal_writestring("\n");
	schedule();
	curr_task = next_task;
	resume_task();
}

void schedule()
{
	schedule_switch_flag = 0;
	uint64_t time_diff = clock_time_diff();
	
	if(curr_task)
	{
		curr_task->time_slice -= time_diff;
		terminal_writeprocID(curr_task->pid);
	}
	
	if(!curr_task || curr_task->state == TSK_Terminated || curr_task->time_slice<=0 || curr_task->state == TSK_Exited || curr_task->state == TSK_Sleeping)
	{
		schedule_switch_flag = 1;
	}
	
	uint64_t current_prio = 0;
	next_task = 0;
	
	if(schedule_switch_flag)
	{
		for(uint16_t index = 1; index<SCHEDULER_MAX_TASKS; index++)
		{
			if((uint32_t)tasks[index]<=SCHEDULER_MAX_TASKS)
			{
				if(tasks[index])
					index = (uint32_t)tasks[index]-1;
				else
					break;
			}
			//If we are asleep and waiting on a lock that has been freed
			//we set the process to waiting and decrement index so on the next loop it gets processed properly
			else if(tasks[index]->state == TSK_Sleeping && tasks[index]->waiting_on && tasks[index]->waiting_on->locked == 0)
			{
				tasks[index]->state = TSK_Waiting;
				index--;
			}
			else if(tasks[index]->state == TSK_Waiting)
			{
				tasks[index]->priority += tasks[index]->priority_mod + 1;
				if(tasks[index]->priority > current_prio)
				{
					next_task = tasks[index];
					current_prio = next_task->priority;
				}
			}
		}
		if(next_task)
		{
			next_task->priority = 0;
			next_task->time_slice = SCHEDULER_TIMESLICE;
			curr_task = next_task;
		}
		else
		{
			next_task = kernel_task;
			curr_task = next_task;
		}
	}
}

/*
void schedule()
{
	schedule_switch_flag=0;
	uint64_t time_diff = clock_time_diff();

	if(curr_task)
	{	
		//terminal_writeuint64(curr_task->time_slice);
		curr_task->time_slice -= time_diff;
		//terminal_writeuint64(curr_task->time_slice);
	}
	
	if(!curr_task || curr_task->state == TSK_Terminated || curr_task->time_slice<=0 || curr_task->state == TSK_Exited)
	{
		schedule_switch_flag = 1;					//We want to switch if our current process is done for now
		curr_task->priority=0;
		//terminal_writestring("Switching ");
		//terminal_writeuint32(curr_task->pid);
	}
	
	uint64_t current_prio = 0;
	next_task=0;
	
	for(uint16_t index=1; index<SCHEDULER_MAX_TASKS; index++)
	{
		if((uint32_t)tasks[index]<=SCHEDULER_MAX_TASKS)	//Tells us to jump forward
		{
			//Go to next entry
			if(tasks[index])
				index = (uint32_t)tasks[index]-1;
			//If there is no next entry we get ready to switch
			else if(schedule_switch_flag)
			{
				if(next_task == 0)
				{
					if(curr_task->state == TSK_Terminated || curr_task->state == TSK_Exited)
					{
						next_task = kernel_task;
						schedule_switch_flag = 1;
					}
					else
						schedule_switch_flag=0;
				}
				else if(next_task!=curr_task)
				{
					next_task->time_slice = next_task->priority;
					//terminal_writeuint32(next_task->pid);
					//terminal_writeuint64(next_task->time_slice);
					next_task->priority=0;
					curr_task = next_task;
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
*/