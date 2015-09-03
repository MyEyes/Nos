#ifndef _IPC_PORT_H
#define _IPC_PORT_H
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <ipc/msg.h>
#include <lock.h>
#define IPC_PORT_MAX 254
typedef enum
{
	IPC_PORT_UNINITIALIZED = 0,
	IPC_PORT_OPEN,
	IPC_PORT_CLOSED
} ipc_port_state;

typedef struct
{
	void* buffer_base;
	size_t buffer_size;
	uint32_t buffer_offset;
	size_t buffer_filled;
} ipc_buffer_t;

typedef struct
{
	ipc_port_state state;
	pid_t owner_pid;
	lock_t lock;
	ipc_buffer_t* buffer;
} ipc_port_t;


ipc_buffer_t* req_port(uint32_t);

ipc_port_t* init_port(uint32_t, size_t);
int open_port(uint32_t);
int close_port(uint32_t);
int free_port(uint32_t);

int get_free_port();

void* get_ipc_res_buffer(size_t);

int get_ipc_message(uint32_t, ipc_msg_hdr_t* header, void* data, size_t numbytes);
int send_to_buffer(ipc_buffer_t*,uint32_t, const void*, size_t);
int send_to_port(uint32_t,uint32_t, const void*, size_t);
int yield_control_to_port(uint32_t port);
int copy_to_buffer(ipc_buffer_t*, const void*, size_t);
#endif