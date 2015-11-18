#ifndef FILE_SYS_H_
#define FILE_SYS_H_

#define FS_CMD_GETFILES 0
#define FS_CMD_READFILE 1
#define FS_CMD_WRITEFILE 2
#define FS_CMD_CREATEFILE 3
#define FS_CMD_DELETEFILE 4
#define FS_CMD_GETFILE 5

#include <stdio.h>


typedef struct
{
	FILE dir;
	int offset;
	int buffsize;
} fs_cmd_readdir_parm;

typedef struct
{
	FILE dir;
	char filename[];
} fs_cmd_getfile_parm;

typedef struct
{
	FILE id;
	int offset;
	int buffsize;
} fs_cmd_readfile_parm;

typedef struct
{
	FILE id;
	int offset;
	int buffsize;
	char data[];
} fs_cmd_writefile_parm;

typedef struct
{
	FILE dir;
	char filename[];
} fs_cmd_createfile_parm;

typedef struct
{
	FILE dir;
	char filename[];
} fs_cmd_createdir_parm;

typedef struct
{
	FILE dir;
} fs_cmd_delfile_parm;

#endif