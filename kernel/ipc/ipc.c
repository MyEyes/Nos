#include <ipc/port.h>
#include <ipc/msg.h>
#include <ipc/ipc.h>
#include <kalloc.h>
#include <scheduler.h>
#include <unistd.h>
#include <string.h>

ipc_port_t* ports;
int initialized = 0;

void init_ipc()
{
	ports = (ipc_port_t*) kalloc(sizeof(ipc_port_t)*IPC_PORT_MAX);
	memzero(ports, sizeof(ipc_port_t)*IPC_PORT_MAX);
	initialized = 1;
}

ipc_port_t* init_port(uint32_t port, size_t buffersize)
{
	if(initialized && port<IPC_PORT_MAX)
	{
		acquire_lock(&(ports[port].lock));
		if( ports[port].state == IPC_PORT_UNINITIALIZED)
		{
			ports[port].state = IPC_PORT_CLOSED;
			ports[port].owner_pid = get_current_pid();
			ports[port].buffer = (ipc_buffer_t*) kalloc(sizeof(ipc_buffer_t));
			ports[port].buffer->buffer_base = kalloc(buffersize);
			ports[port].buffer->buffer_size = buffersize;
			ports[port].buffer->buffer_offset = 0;
			ports[port].buffer->buffer_filled = 0;
			release_lock(&(ports[port].lock));
			return ports + port;
		}
	}
	release_lock(&(ports[port].lock));
	return 0;
}

int open_port(uint32_t port)
{
	if(!initialized)
		return -1;
	if(ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	pid_t pid = get_current_pid();
	if(ports[port].owner_pid != pid)
		return -1;
	ports[port].state = IPC_PORT_OPEN;
	return 0;
}

int close_port(uint32_t port)
{
	if(!initialized)
		return -1;
	if(ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	pid_t pid = get_current_pid();
	if(ports[port].owner_pid != pid)
		return -1;
	ports[port].state = IPC_PORT_CLOSED;
	return 0;
}

int free_port(uint32_t port)
{
	if(!initialized)
		return -1;
	if(ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	pid_t pid = get_current_pid();
	if(ports[port].owner_pid != pid)
		return -1;
	ports[port].state = IPC_PORT_UNINITIALIZED;
	kfree(ports[port].buffer->buffer_base);
	kfree(ports[port].buffer);
	return 0;
}

int send_to_port(uint32_t port, uint32_t replyport, const void* data, size_t numbytes)
{
	if(!initialized)
		return -1;
	acquire_lock(&(ports[port].lock));
	if(ports[port].state != IPC_PORT_OPEN)
	{
		release_lock(&(ports[port].lock));
		return -1;
	}
	int res = send_to_buffer(ports[port].buffer, replyport, data, numbytes);
	signal(ports[port].owner_pid);
	release_lock(&(ports[port].lock));
	return res;
}

int send_to_buffer(ipc_buffer_t* buffer, uint32_t replyport, const void* data, size_t numbytes)
{
	//If the message doesn't fit in the buffer
	if(buffer->buffer_size - buffer->buffer_filled < numbytes + sizeof(ipc_msg_hdr_t))
		return -1;
	ipc_msg_hdr_t dummy;
	dummy.sender_pid = get_current_pid();
	dummy.reply_port = replyport;
	dummy.msg_size = numbytes;
	copy_to_buffer(buffer, &dummy, sizeof(ipc_msg_hdr_t));
	copy_to_buffer(buffer, data, numbytes);
	return 0;
}

int copy_to_buffer(ipc_buffer_t* buffer, const void* data, size_t numbytes)
{
	int overhang = numbytes + buffer->buffer_offset - buffer->buffer_size;
	
	//If we need to wrap around
	if(overhang>0)
	{
		//Figure out how much still fits into buffer
		uint32_t bytesFirst = buffer->buffer_size-buffer->buffer_offset;
		//Copy up to there
		copy_to_buffer(buffer, data, bytesFirst);
		//Copy the rest after the buffer wraps around
		copy_to_buffer(buffer, data+bytesFirst, numbytes-bytesFirst);
		return 0;
	}
	memcpy(buffer->buffer_base+buffer->buffer_offset, data, numbytes);
	buffer->buffer_offset+=numbytes;
	buffer->buffer_filled+=numbytes;
	if(buffer->buffer_offset==buffer->buffer_size)
		buffer->buffer_offset=0;
	return 0;
}

int copy_from_buffer(ipc_buffer_t* buffer, void* data, size_t numbytes)
{
	if(numbytes==0)
		return 0;
	//Figure out where we need to start copying from
	int start = buffer->buffer_offset-buffer->buffer_filled;
	if(start<0)
		start += buffer->buffer_size;
	int overhang = numbytes + start - buffer->buffer_size;
	
	//If we need to wrap around
	if(overhang>0)
	{
		//Figure out how much still fits into buffer
		uint32_t bytesFirst = buffer->buffer_size-start;
		//Copy up to there
		copy_from_buffer(buffer, data, bytesFirst);
		//Copy the rest after the buffer wraps around
		copy_from_buffer(buffer, data+bytesFirst, numbytes-bytesFirst);
		return 0;
	}
	memcpy(data, buffer->buffer_base+start, numbytes);
	buffer->buffer_filled-=numbytes;
	return 0;
}

int get_ipc_message(uint32_t port, ipc_msg_hdr_t* header, void* data, size_t numbytes)
{
	acquire_lock(&(ports[port].lock));
	ipc_buffer_t* buffer = ports[port].buffer;
	if(buffer->buffer_filled<=0)
	{
		release_lock(&(ports[port].lock));
		return -1;
	}
	copy_from_buffer(buffer, header, sizeof(ipc_msg_hdr_t));
	int size = numbytes;
	if(numbytes>header->msg_size)
		size=header->msg_size;
	copy_from_buffer(buffer, data, size);
	release_lock(&(ports[port].lock));
	return 0;
}