#ifndef COMMAND_H_
#define COMMAND_H_
typedef struct
{
	char name[16];
	int (*func)(int, char**);
} command_t;
#endif