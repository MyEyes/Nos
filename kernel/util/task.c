#include "task.h"
#include "../res/tss.h"
#include "../res/gdt.h"
#include "../res/kalloc.h"
#include "../res/paging.h"
#include "../res/mem.h"

void (*target)();
uint32_t old_stack;
uint32_t target_stack;
extern void jump_usermode();
extern void switch_context();

//We should be in usermode here
void user_mode_entry()
{
	target();
}

task_t* create_task(void (*entry)(), uint16_t ss, uint16_t cs, uint16_t ds)
{
	task_t* new_task = kalloc(sizeof(task_t));
	new_task->entry = entry;
	new_task->esp = ((uint32_t)new_task)+PAGE_SIZE;
	
	task_context_t* stack = (task_context_t*)(new_task->esp-sizeof(task_context_t));
	memzero((void*)stack, sizeof(stack));
	stack->ss = ss;
	if(cs&3)
		stack->flags = 512;
	else
		stack->flags = 0;
	stack->esp = (uint32_t)stack;
	stack->cs = cs;
	stack->ds = ds;
	stack->es = ds;
	stack->fs = ds;
	stack->gs = ds;
	stack->entry = (uint32_t)entry;
	new_task->esp = stack->esp;
	return new_task;
}

void call_task(task_t* task)
{
	target_stack = task->esp;
	old_stack=0;
	//We are switching context so we won't return here
	//So we pop the passed parameter and return address
	//to keep the stack clean
	__asm__ (	"pop %%eax\n\r"
				"pop %%eax\n\r"
				:
				:
				:"memory");
	switch_context();
}

void switch_task(task_t* oldtask, task_t* newtask)
{
	if(oldtask==newtask)
		return;
	//We will never return so the calling 
	//function can't clean up the stack.
	//So we pop the two passed parameters and return address
	//away before calling switch_context
	old_stack = (uint32_t)&(oldtask->esp);
	target_stack = newtask->esp;
	__asm__ (	"pop %%eax\n\r"
				"pop %%eax\n\r"
				"pop %%eax\n\r"
				:
				:
				:"memory");
	switch_context();
}