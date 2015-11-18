#ifndef CMD_H_
#define CMD_H_

#define MAX_CMD_LENGTH 256

#define SET_COMMAND(num, cname, function){									\
											strcpy(cmds[num].name, cname);	\
											cmds[num].func = function;		\
										}

int get_command();
int proc_cmd();
int param_split(char*, char***);

int cmd_exit(int, char**);
int cmd_echo(int, char**);
int cmd_crash(int, char**);
int cmd_getfile(int, char**);
int cmd_cd(int, char**);
#endif