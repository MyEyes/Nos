#include <stdio.h>
#include <stdlib.h>
#include <ipc/ipc.h>
#include <string.h>
#include "cmd.h"
#include "command.h"
#include <debug.h>

char input_buffer[MAX_CMD_LENGTH];
int input_buffer_pos;
#define nCmds 5
command_t cmds[nCmds];
char term = 0;

int main(int argc, char** argv)
{
	//Avoid warnings
	argc=argc;
	argv=argv;
	
	print("Welcome to the Nos command line!\n");
	
	SET_COMMAND(0,"exit", cmd_exit);
	SET_COMMAND(1,"echo", cmd_echo);
	SET_COMMAND(2,"crash", cmd_crash);
	SET_COMMAND(3,"getfile", cmd_getfile);
	SET_COMMAND(4,"cd", cmd_cd);
	bochs_break();
	while(!term)
		get_command();
	return 0;
}

int get_command()
{
	input_buffer_pos = 0;
	print("\n");
	char outbuf[2];
	outbuf[1] = 0;
	outbuf[0] = '>';
	print(outbuf);
	char c;
	do
	{
		c = getc();
		if(c)
		{			
			if(c=='\b' && input_buffer_pos>0)
			{
				outbuf[0]=c;
				printf(outbuf);
				input_buffer[input_buffer_pos--] = 0;
			}
			else if(c=='\n' && input_buffer_pos<MAX_CMD_LENGTH-1)
			{
				outbuf[0]=c;
				printf(outbuf);
				input_buffer[input_buffer_pos] = 0;
				return proc_cmd();
			}
			else if(c>=32 && input_buffer_pos<MAX_CMD_LENGTH-2)
			{
				input_buffer[input_buffer_pos++] = c;
				outbuf[0]=c;
				printf(outbuf);
			}
			
		}
	}while(1);
}

int proc_cmd()
{
	char** cmd_args;
	int nargs = param_split(input_buffer, &cmd_args);
	if(nargs==0)
	{
		free(cmd_args);
		return 0;
	}
	for(int x=0; x<nCmds; x++)
	{
		if(strcmp(cmds[x].name, cmd_args[0])==0)
		{
			int res = cmds[x].func(nargs-1, cmd_args+1);
			free(cmd_args);
			return res;
		}
	}
	printf("Command %s not found!", cmd_args[0]);
	free(cmd_args);
	return 0;
}

int param_split(char* input, char*** args)
{
	int count = 0;
	char param = 0;
	int offset = 0;
	while(input[offset])
	{
		if(!param && input[offset]!=' ')
		{
			param = 1;
			count++;
		}
		else if(param && input[offset]==' ')
		{
			param = 0;
		}
		offset++;
	}
	*args = (char**)calloc(count, sizeof(char*));
	if(!*args)
	{
		print("Couldn't allocate arg buffer\n");
		return 0;
	}
	offset = 0;
	count = 0;
	param = 0;
	while(input[offset])
	{
		if(!param && input[offset]!=' ')
		{
			param = 1;
			bochs_break();
			(*args)[count++]=input+offset;
		}
		else if(param && input[offset]==' ')
		{
			param = 0;
		}
		if(input[offset]==' ')
			input[offset] = 0;
		offset++;
	}
	return count;
}

int cmd_exit(int nargs, char** vargs)
{
	//Avoid warnings
	nargs=nargs;
	vargs=vargs;
	
	term = 1;
	return 0;
}

int cmd_echo(int nargs, char** vargs)
{
	
	for(int x=0; x<nargs; x++)
	{
		printf("%s ", vargs[x]);
	}
	return 0;
}

int cmd_crash(int nargs, char** vargs)
{
	//Avoid warnings
	nargs=nargs;
	vargs=vargs;
	
	*((char*)0)=0;
	return 0;
}

int cmd_getfile(int nargs, char** vargs)
{
	if(nargs!=1)
	{
		printf("Invalid number of arguments %d", nargs);
		return 0;
	}
	FILE* file = fopen(*vargs, "");
	if(file)
		printf("inode: %d\ntype:  %d\n", file->handle, file->type);
	else
		printf("Couldn't find inode for %s\n", *vargs);
	return 0;
}

int cmd_cd(int nargs, char** vargs)
{
	if(nargs!=1)
	{
		printf("Invalid number of arguments %d", nargs);
		return 0;
	}
	FILE* file = fopen(*vargs, "");
	if(file)
	{
		if(file->type==FILE_TYPE_DIR)
			set_dir(file);
		else
			printf("%s is not a directory\n", *vargs);
	}
	else
		printf("Couldn't find inode for %s\n", *vargs);
	return 0;
}