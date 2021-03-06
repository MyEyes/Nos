#ifndef SCHEDULER_H
#define SCHEDULER_H
#include <task.h>
#include <sys/types.h>
#define SCHEDULER_MAX_TASKS 256
#define SCHEDULER_TIMESLICE 0x2000000000

typedef struct task_list_st task_list_t;
struct task_list_st
{
	task_t* task;
	task_list_t* prev;
	task_list_t* next;
};

void scheduler_spawn(task_t* task);
void schedule_kill();
void schedule_task_stop();
extern __attribute__((noreturn)) void resume_task();

extern task_t* kernel_task;

void schd_task_add(task_t* task);
void schd_task_del(task_t* task);
void init_scheduler();
void schedule();
void __attribute__((noreturn)) schedule_exit();
pid_t get_current_pid();
task_t* get_current_task();
task_t* get_task(pid_t pid);

void print_current_task();
#endif