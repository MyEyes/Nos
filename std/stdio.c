#include <stdio.h>
#include <ipc/ipc.h>
#include <ipc/port.h>
#include <string.h>
#include <drv_tab.h>
#include <paging.h>
#include <stdlib.h>
#include <format.h>
#include <stdarg.h>
#include <drv/file_sys.h>

FILE root_file;
FILE curr_dir;

int print(const char* str)
{
	int res = send_to_port(IPC_PORT_TERMINAL, IPC_PORT_NONE, str, strlen(str)+1);
	yield_control_to_port(IPC_PORT_TERMINAL);
	return res;
}	

int printf(const char *format, ...)
{
	va_list ap;
	va_start(ap, format);
	size_t len = _format_length(format, &ap);
	va_end(ap);
	char* buf = (char*)malloc(len+1);
	va_start(ap, format);
	_sprintf(buf, format, &ap);
	int res = puts(buf);
	free(buf);
	va_end(ap);
	return res;
}

FILE fs_open(FILE dir, const char *path)
{
	//Set up a port to receive the result
	int portnum = get_free_port();
	init_port(portnum, 128);
	open_port(portnum);
	
	//Have required memory on stack
	ipc_msg_hdr_t curr_msg;
	FILE result;
	result.handle = -1;
	
	char* cmd_buf = malloc(sizeof(char)+sizeof(fs_cmd_getfile_parm)+(strlen(path)+1)*sizeof(char));
	*cmd_buf = FS_CMD_GETFILE;
	fs_cmd_getfile_parm* parm = (fs_cmd_getfile_parm*) (cmd_buf+1);
	parm->dir = dir;
	memcpy(parm->filename, path, strlen(path)+1);
	int res = send_to_port(IPC_PORT_FILESYSTEM, portnum, cmd_buf, sizeof(fs_cmd_getfile_parm)+(strlen(path)+2)*sizeof(char));
	if(res==0)
	{
		while(get_ipc_message(portnum, &curr_msg, (char*)&result, sizeof(FILE))<0)
			yield_control_to_port(IPC_PORT_FILESYSTEM);
	}
	else
		print("Couldn't send to fs port\n");
	free_port(portnum);
	free(parm);
	return result;
}

FILE* fopen(char *path, const char *flags)
{
	//Avoid warning
	flags=flags;
	
	//Start at current directory
	FILE curr_file = curr_dir;
	
	char* last_path;
	char* curr_path = path;
	
	//Do we have an absolute path?
	if(*curr_path=='/')
	{
		curr_file = root_file;
		curr_path++;
	}
	
	//Go until the end of the given path
	while(*curr_path)
	{
		//Remember where we started reading for the current part
		last_path = curr_path;
		//Read until the string ends or we find a separator
		while(*curr_path && *curr_path!='/')
		{
			curr_path++;
		}
		//Replace *current_path with 0 so that we terminate the partial path
		if(*curr_path=='/')
		{
			*curr_path=0;
			curr_path++;
		}
		curr_file = fs_open(curr_file, last_path);
		//If we couldn't open the partial path return 0
		if(curr_file.handle == (unsigned int)-1)
			return 0;
	}
	//Otherwise we allocate storage and return the current file
	FILE* result = malloc(sizeof(FILE));
	*result = curr_file;
	return result;
}

int set_dir(FILE* dir)
{
	if(dir->type!=FILE_TYPE_DIR)
		return -1;
	curr_dir = *dir;
	return 0;
}

int fclose(FILE* file)
{
	free(file);
	return 0;
}

int puts(const char *s)
{
	return print(s);
}

char getc()
{
	int portnum = get_free_port();
	init_port(portnum, 128);
	open_port(portnum);
	char cmd[1];
	cmd[0] = 1;
	ipc_msg_hdr_t curr_msg;
	int res = send_to_port(IPC_PORT_KEYBOARD, portnum, (char*)cmd, 1);
	if(res==0)
	{
		while(get_ipc_message(portnum, &curr_msg, (char*)cmd, 1)<0)	
			yield_control_to_port(IPC_PORT_KEYBOARD);
	}
	else
	{
		print("Couldn't send to keyboard port\n");
	}
	free_port(portnum);
	return cmd[0];
}