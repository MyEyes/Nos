#include <stdio.h>
#include <stdlib.h>
#include <ipc/ipc.h>
#include <string.h>
#include "cmd.h"
#include "command.h"

char input_buffer[MAX_CMD_LENGTH];
int input_buffer_pos;
#define nCmds 1
command_t cmds[nCmds];

int main(int argc, char** argv)
{
	print("Welcome to the Nos command line!\n\n");
	
	SET_COMMAND(0,"exit", cmd_exit);
	while(!get_command()) ;
	return 0;
}

int get_command()
{
	input_buffer_pos = 0;
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
			else if(c=='\n')
			{
				outbuf[0]=c;
				printf(outbuf);
				input_buffer[input_buffer_pos] = 0;
				return proc_cmd();
			}
			else if(c>=32 && input_buffer_pos<MAX_CMD_LENGTH-1)
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
	printf("Command %s not found!\n", cmd_args[0]);
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
	return -1;
}