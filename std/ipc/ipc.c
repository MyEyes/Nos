#include <ipc/port.h>
#include <ipc/msg.h>
#include <ipc/ipc.h>
#include <int.h>
#include <idt.h>
#include <paging.h>
#include <kernel.h>
#include <stddef.h>
#include <scheduler.h>
#include <unistd.h>
#include <string.h>

ipc_port_t* ipc_ports;
ipc_buffer_t* ipc_buffer_tbl;
char* ipc_res_buffer_map;
void* ipc_buffer_start;
int ipc_initialized = 0;
int ipc_buffer_pages;

void init_ipc()
{
	int port_info_tab_sz = sizeof(ipc_port_t)*IPC_PORT_MAX;
	ipc_ports = (ipc_port_t*)KMEM_IPC_EXCL_LOC;
	
	ipc_buffer_tbl = (ipc_buffer_t*)(KMEM_IPC_EXCL_LOC + port_info_tab_sz);
	ipc_res_buffer_map = (char*)ipc_buffer_tbl + IPC_PORT_MAX * sizeof(ipc_buffer_t);
	//Assume one byte per page in buffer reserved area
	//Round up ipc_buffer_start to next page boundary
	ipc_buffer_start = ipc_res_buffer_map + (KMEM_IPC_EXCL_SIZE)/PAGE_SIZE; 
	ipc_buffer_start += PAGE_SIZE - 1;
	ipc_buffer_start = (void*)((uint32_t)ipc_buffer_start&0xFFFFF000);
	
	ipc_buffer_pages = KMEM_IPC_EXCL_SIZE / PAGE_SIZE;
	
	ipc_initialized = 1;
}

void* alloc_ipc_res_buffer(size_t sz, uint32_t port)
{
	int needed_pgs = (sz+PAGE_SIZE-1)/PAGE_SIZE; //Round up
	int found_pgs = 0;
	for(int x=0; x<ipc_buffer_pages; x++)
	{
		if(ipc_res_buffer_map[x] == 0)
		{
			found_pgs++;
			if(found_pgs==needed_pgs)
			{
				for(int y=x; found_pgs>0; y--, found_pgs--)
					ipc_res_buffer_map[y] = port;
				return (ipc_buffer_start+(x-needed_pgs+1)*PAGE_SIZE);
			}
		}
		else
			found_pgs = 0;
	}
	return 0;
}

void free_ipc_res_buffer(void* loc)
{
	int id = ((uint32_t)loc-(uint32_t)ipc_buffer_start)/PAGE_SIZE;
	if(id<0 || id>=ipc_buffer_pages)
		return;
	int val = ipc_res_buffer_map[id];
	if(val<=0)
		return;
	for(; id<ipc_buffer_pages && ipc_res_buffer_map[id]==val; id++)
		ipc_res_buffer_map[id] = 0;
}

ipc_port_t* init_port(uint32_t port, size_t buffersize)
{
	if(ipc_initialized && port<IPC_PORT_MAX)
	{
		acquire_lock(&(ipc_ports[port].lock));
		if(ipc_ports[port].state == IPC_PORT_UNINITIALIZED)
		{
			ipc_ports[port].state = IPC_PORT_CLOSED;
			ipc_ports[port].owner_pid = get_current_pid();
			ipc_ports[port].buffer = ipc_buffer_tbl + port;
			ipc_ports[port].buffer->buffer_base = alloc_ipc_res_buffer(buffersize, port);
			ipc_ports[port].buffer->buffer_size = buffersize;
			ipc_ports[port].buffer->buffer_offset = 0;
			ipc_ports[port].buffer->buffer_filled = 0;
			release_lock(&(ipc_ports[port].lock));
			return ipc_ports + port;
		}
	}
	release_lock(&(ipc_ports[port].lock));
	return 0;
}

int get_free_port()
{
	if(ipc_initialized)
	{
		for(int x=1; x<IPC_PORT_MAX; x++)
		{
			if(ipc_ports[x].state == IPC_PORT_UNINITIALIZED)
			{
				return x;
			}
		}
		return -1;
	}
	else
		return -1;
}

int open_port(uint32_t port)
{
	if(!ipc_initialized)
		return -1;
	if(ipc_ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	pid_t pid = get_current_pid();
	if(ipc_ports[port].owner_pid != pid)
		return -1;
	ipc_ports[port].state = IPC_PORT_OPEN;
	return 0;
}

int close_port(uint32_t port)
{
	if(!ipc_initialized)
		return -1;
	if(ipc_ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	pid_t pid = get_current_pid();
	if(ipc_ports[port].owner_pid != pid)
		return -1;
	ipc_ports[port].state = IPC_PORT_CLOSED;
	return 0;
}

int free_port(uint32_t port)
{
	if(!ipc_initialized)
		return -1;
	if(ipc_ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	pid_t pid = get_current_pid();
	if(ipc_ports[port].owner_pid != pid)
		return -1;
	ipc_ports[port].state = IPC_PORT_UNINITIALIZED;
	free_ipc_res_buffer(ipc_ports[port].buffer->buffer_base);
	return 0;
}

int yield_control_to_port(uint32_t port)
{
	if(!ipc_initialized)
		return -1;
	if(ipc_ports[port].state == IPC_PORT_UNINITIALIZED)
		return -1;
	__asm__ ("mov %0, %%eax"::"S"(port):"memory");
	call_int(PROC_YIELD);
	return 0;
}

int send_to_port(uint32_t port, uint32_t replyport, const void* data, size_t numbytes)
{
	if(!ipc_initialized)
		return -1;
	acquire_lock(&(ipc_ports[port].lock));
	if(ipc_ports[port].state != IPC_PORT_OPEN)
	{
		release_lock(&(ipc_ports[port].lock));
		return -1;
	}
	int res = send_to_buffer(ipc_ports[port].buffer, replyport, data, numbytes);
	signal(ipc_ports[port].owner_pid);
	release_lock(&(ipc_ports[port].lock));
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
	acquire_lock(&(ipc_ports[port].lock));
	ipc_buffer_t* buffer = ipc_ports[port].buffer;
	if(buffer->buffer_filled<=0)
	{
		release_lock(&(ipc_ports[port].lock));
		return -1;
	}
	copy_from_buffer(buffer, header, sizeof(ipc_msg_hdr_t));
	int size = numbytes;
	if(numbytes>header->msg_size)
		size=header->msg_size;
	copy_from_buffer(buffer, data, size);
	release_lock(&(ipc_ports[port].lock));
	return 0;
}