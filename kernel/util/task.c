#include "task.h"
#include "../res/tss.h"
#include "../res/gdt.h"
#include "../res/kalloc.h"
#include "../res/paging.h"
#include "../res/mem.h"
#include "../res/gdt.h"

void (*target)();
uint32_t old_stack;
uint32_t target_stack;

//All define in ring3.s
extern void jump_usermode();
extern void switch_context();
extern void switch_context_nolevel();
extern void no_switch();

//We should be in usermode here
void user_mode_entry()
{
	target();
}

task_t* create_user_task(void (*entry)(), int8_t priority)
{
	return create_task(entry, GDT_USER_DATA_SEG+3, GDT_USER_CODE_SEG+3, GDT_USER_DATA_SEG+3, priority);
}

task_t* create_task(void (*entry)(), uint16_t ss, uint16_t cs, uint16_t ds, int8_t priority)
{
	task_t* new_task = kalloc(sizeof(task_t));
	new_task->entry = entry;
	new_task->esp = ((uint32_t)new_task)+PAGE_SIZE;
	new_task->level = cs&0x3;
	new_task->priority = 0;
	new_task->priority_mod = priority;
	new_task->time_slice=0;
	new_task->state = TSK_Waiting;
	
	task_context_t* stack = (task_context_t*)(new_task->esp-sizeof(task_context_t));
	memzero((void*)stack, sizeof(stack));
	stack->ss = ss;
	if(new_task->level != 0)
		stack->flags = 512;
	else
		stack->flags = 0;
	stack->esp = new_task->esp;
	stack->cs = cs;
	stack->ds = ds;
	stack->es = ds;
	stack->fs = ds;
	stack->gs = ds;
	stack->entry = (uint32_t)entry;
	new_task->esp = (uint32_t) stack;
	return new_task;
}

/* In ASM now
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
*/

/* In ASM now
void switch_task()
{
	//We will never return so the calling 
	//function can't clean up the stack.
	//So we pop the two passed parameters and return address
	//away before calling switch_context
	__asm__ (	""
				""
				: "=a" (old_stack), "=b" (target_stack)
				:
				:(old_stack), (target_stack))
	//old_stack = (uint32_t)&(oldtask->esp);
	//target_stack = newtask->esp;
	__asm__ (	"add $4, %%esp\n\r"
				:
				:
				:"memory", "%esp");
	if(oldtask == newtask)
		no_switch();
	if(oldtask->level != newtask->level)
		switch_context();
	else
		switch_context_nolevel();
}*/