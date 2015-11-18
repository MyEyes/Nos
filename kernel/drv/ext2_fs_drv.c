#include "ext2_fs_drv.h"
#include <scheduler.h>
#include <ipc/ipc.h>
#include <gdt.h>
#include <drv_tab.h>
#include <kalloc.h>
#include "ext2.h"
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <int.h>
#include <terminal.h>

#define IPC_BUF_SIZE 4000

dev_desc_t device;
ext2_hook_t hook_data;
ext2_hook_t* hook;
ipc_port_t* fs_port;
task_t* fs_task;

char* ipc_buffer;

	
void ext2_fs_init(dev_desc_t dev)
{
	device = dev;
	hook_data = ext2_create_hook(&device);
	hook = &hook_data;
	fs_task = create_task(ext2_fs_run, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	scheduler_spawn(fs_task);
	ipc_buffer = kalloc(sizeof(char)*IPC_BUF_SIZE);
}

void ext2_fs_run()
{
	fs_port = init_port(IPC_PORT_FILESYSTEM, IPC_BUF_SIZE);
	open_port(IPC_PORT_FILESYSTEM);
	enable_interrupts();
	
	ipc_msg_hdr_t curr_header;
	while(1)
	{
		if(!get_ipc_message(IPC_PORT_FILESYSTEM, &curr_header, ipc_buffer, IPC_BUF_SIZE))
		{
			switch(*ipc_buffer)
			{
				case FS_CMD_GETFILES: ext2_fs_readdir(curr_header.reply_port, (fs_cmd_readdir_parm*)(ipc_buffer+1)); break;
				case FS_CMD_READFILE: ext2_fs_readfile(curr_header.reply_port, (fs_cmd_readfile_parm*)(ipc_buffer+1)); break;
				case FS_CMD_WRITEFILE: ext2_fs_writefile(curr_header.reply_port, (fs_cmd_writefile_parm*)(ipc_buffer+1)); break;
				case FS_CMD_CREATEFILE: ext2_fs_createfile((fs_cmd_createfile_parm*)(ipc_buffer+1)); break;
				case FS_CMD_DELETEFILE: ext2_fs_delfile((fs_cmd_delfile_parm*)(ipc_buffer+1)); break;
				case FS_CMD_GETFILE: ext2_fs_getfile(curr_header.reply_port, (fs_cmd_getfile_parm*)(ipc_buffer+1)); break;
			}
		}
		else
		{
			sleep();
		}
	}
}

void ext2_fs_getfile(int port, fs_cmd_getfile_parm* parm)
{
	FILE result;
	result.handle = (unsigned int)-1;
	//If there was trouble reading the directory inode
	//We return -1
	if(ext2_read_inode(hook, parm->dir.handle)<0)
	{
		send_to_port(port, IPC_PORT_FILESYSTEM, (char*)&result, sizeof(FILE));
		return;
	}
	//Check that inode is a directory
	//otherwise return -1
	if((hook->inode_buffer->type_perm & 0xF000) != 0x4000)
	{
		send_to_port(port, IPC_PORT_FILESYSTEM, (char*)&result, sizeof(FILE));
		return;
	}
	int bytes_to_read = hook->inode_buffer->size_low;
	int blk_id = 0;
	//Read through the directories entries and check if the name occurs
	while(bytes_to_read>0)
	{
		//Read the inode blocks into the hooks buffers
		int blk = ext2_read_inode_blk(hook, parm->dir.handle, blk_id);
		unsigned int buffer_offset = 0;
		
		//Loop until we run out of stuff to read in this block
		while(buffer_offset<hook->blocksize)
		{
			//Read current block entry
			ext2_dir_entry_t* curr_dir = (ext2_dir_entry_t*)(hook->buffer + blk * hook->blocksize + buffer_offset);
			if(strlen(parm->filename)==curr_dir->name_len && strncmp(parm->filename, curr_dir->name, curr_dir->name_len)==0)
			{
				//Read that files inode to check what type it is
				if(ext2_read_inode(hook, curr_dir->inode)<0)
				{
					//If we have trouble reading the inode we just return not found
					send_to_port(port, IPC_PORT_FILESYSTEM, (char*)&result, sizeof(FILE));
					return;
				}
				
				result.handle = curr_dir->inode;
				result.type = ((hook->inode_buffer->type_perm & 0xF000) == 0x4000) ? FILE_TYPE_DIR:FILE_TYPE_FILE;
				
				if(send_to_port(port, IPC_PORT_FILESYSTEM, (char*)&result, sizeof(FILE))==0)
					yield_control_to_port(port);
				else
					terminal_writestring("Couldn't send to port\n");
				return;
			}
			buffer_offset += curr_dir->total_size;
		}
		//Set up to read the next block
		bytes_to_read-=hook->blocksize;
		blk_id++;
	}
	
	send_to_port(port, IPC_PORT_FILESYSTEM, (char*)&result, sizeof(FILE));
	yield_control_to_port(port);
}

void ext2_fs_readdir(int port, fs_cmd_readdir_parm* parm)
{
	port=port;
	parm=parm;
}

void ext2_fs_readfile(int port, fs_cmd_readfile_parm* parm)
{
	port=port;
	parm=parm;
}

void ext2_fs_writefile(int port, fs_cmd_writefile_parm* parm)
{
	parm=parm;
	port=port;
}

void ext2_fs_createfile(fs_cmd_createfile_parm* parm)
{
	ext2_create_empty_file(hook, parm->dir.handle, parm->filename);
}

void ext2_fs_createdir(fs_cmd_createdir_parm* parm)
{
	ext2_create_empty_dir(hook, parm->dir.handle, parm->filename);
}

void ext2_fs_delfile(fs_cmd_delfile_parm* parm)
{
	parm=parm;
}