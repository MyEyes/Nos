#ifndef TASK_H
#define TASK_H
#include <stdint.h>

typedef enum
{
	TSK_Running,
	TSK_Waiting,
	TSK_Paused
} task_state;

typedef struct
{
	uint32_t esp;
	uint8_t level;
	
	task_state state;
	
	uint64_t priority;
	int8_t priority_mod;
	
	int64_t time_slice;
	void (*entry)();
} task_t;

typedef struct
{
	uint32_t gs;
	uint32_t fs;
	uint32_t es;
	uint32_t ds;
	
	uint32_t edi;
	uint32_t esi;
	uint32_t ebp;
	uint32_t ebx;
	uint32_t edx;
	uint32_t ecx;
	uint32_t eax;
	
	uint32_t entry;
	uint32_t cs;
	uint32_t flags;
	uint32_t esp;
	uint32_t ss;
} __attribute__((packed)) task_context_t;

void call_user(task_t* task);
task_t* create_user_task(void (*entry)(), int8_t priority);
task_t* create_task(void (*entry)(), uint16_t ss, uint16_t cs, uint16_t ds, int8_t priority);
__attribute__((noreturn)) void call_task(task_t* task);
__attribute__((noreturn)) void switch_task(task_t* oldtask, task_t* newtask);
#endif