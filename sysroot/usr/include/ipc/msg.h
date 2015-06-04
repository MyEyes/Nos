#ifndef _IPC_MSG_H
#define _IPC_MSG_H
#include <stdint.h>
#include <sys/types.h>

typedef struct
{
	pid_t sender_pid;
	uint32_t  reply_port;
	size_t msg_size;
} ipc_msg_hdr_t;
#endif