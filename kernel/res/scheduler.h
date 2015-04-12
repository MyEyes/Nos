#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "../util/task.h"

#define SCHEDULER_MAX_TASKS 256

typedef struct task_list_st task_list_t;
struct task_list_st
{
	task_t* task;
	task_list_t* prev;
	task_list_t* next;
};

void scheduler_spawn(task_t* task);
void schd_task_add(task_t* task);
void schd_task_del(task_t* task);
void init_scheduler();

#endif