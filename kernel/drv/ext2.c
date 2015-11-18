#include "ext2.h"
#include <kalloc.h>
#include <string.h>
#include <terminal.h>

ext2_hook_t ext2_create_hook(dev_desc_t* device)
{
	ext2_hook_t result;
	result.device = device;
	
	//Read superblock, potentially with extension
	result.superblock = (ext2_superblock_t*) kalloc(EXT2_SUPERBLOCK_SIZE);
	result.superblock_ext = (ext2_superblock_ext_t*)(result.superblock + 1);
	memzero(result.superblock, EXT2_SUPERBLOCK_SIZE); //Zero out memory, so that if superblock_ext doesn't exist we're fine
	device->read_op(EXT2_SUPERBLOCK_LOC, result.superblock, EXT2_SUPERBLOCK_SIZE, device->dev_struct);
	
	result.valid = (result.superblock->ext_sig == EXT2_SIGNATURE);
	//If this isn't an ext2 partition
	//return early
	if(!result.valid)
		return result;
	
	uint16_t inode_size = result.superblock_ext->inode_size;
	if(inode_size == 0) //If superblock_ext doesn't exist, revert to default inode size
		inode_size = 128;
	result.inode_buffer = kalloc(inode_size);
	result.curr_inode = -1;
	result.inode_size = inode_size;
	
	result.blocksize = 1024 << result.superblock->blk_size; //Calculate blocksize in bytes
	result.buffer = (char*) kalloc(result.blocksize * EXT2_BLOCK_BUFFER_SIZE); //Allocate buffer
	//Zero out buffer index
	for(int x=0; x<EXT2_BLOCK_BUFFER_SIZE; x++)
		result.buffer_index[x] = -1;
	result.next_buffer = 0;
	
	int num_blockgroups = (result.superblock->block_total+result.superblock->blk_p_grp - 1) / result.superblock->blk_p_grp;
	int total_blockgroup_size = num_blockgroups*sizeof(ext2_blk_group_desc_t);
	int blockgroup_blocks = (total_blockgroup_size + result.blocksize - 1) / result.blocksize; //Guarantee rounding up
	
	result.blk_groups = (ext2_blk_group_desc_t*) kalloc(blockgroup_blocks*result.blocksize);
	result.num_blk_groups = num_blockgroups;
	result.blockgroup_blocks = blockgroup_blocks;
	device->read_op(EXT2_SUPERBLOCK_LOC+EXT2_SUPERBLOCK_SIZE, result.blk_groups, blockgroup_blocks*result.blocksize, device->dev_struct);
	result.next_buffer = 0;
	return result;
}

int ext2_read_blk(ext2_hook_t* hook, uint32_t blk_id)
{
	if(!hook->valid)
		return -1;
	
	if(hook->superblock->block_total <= blk_id)
		return -1;
	
	for(int x=0; x<EXT2_BLOCK_BUFFER_SIZE; x++)
		if(hook->buffer_index[x] == blk_id)
			return x;
		
	if(hook->device->read_op((void*)(blk_id * hook->blocksize), hook->buffer+(hook->next_buffer*hook->blocksize), hook->blocksize, hook->device->dev_struct)<0)
		return -1;
	
	hook->buffer_index[hook->next_buffer] = blk_id;
	int res = hook->next_buffer;
	hook->next_buffer++;
	if(hook->next_buffer>=EXT2_BLOCK_BUFFER_SIZE)
		hook->next_buffer = 0;
	return res;
}

int ext2_write_blk(ext2_hook_t* hook, uint32_t blk_id, char* data)
{
	if(!hook->valid)
		return -1;
	
	if(hook->superblock->block_total <= blk_id)
		return -1;
		
	if(hook->device->write_op((void*)(blk_id * hook->blocksize), data, hook->blocksize, hook->device->dev_struct)<0)
		return -1;
	
	return 0;
}

int ext2_read_inode(ext2_hook_t* hook, uint32_t inode_id)
{
	if(!hook->valid)
		return -1;
	if(hook->superblock->inode_total <= inode_id)
		return -1;
	
	if(hook->curr_inode == inode_id)
		return 0;
	
	int blk_grp = (inode_id - 1) / hook->superblock->ins_p_grp; //Containing blockgroup
	int id = (inode_id - 1) % hook->superblock->ins_p_grp;		//Id in blockgroup
	int cb_offset = (id*hook->inode_size) / hook->blocksize;	//offset from first inode table block
	int inodes_per_block = hook->blocksize / hook->inode_size;
	int offset_in_block = (id%inodes_per_block)*hook->inode_size; //offset inside table block
	int inode_tbl_blk = hook->blk_groups[blk_grp].inode_table_blk; //first inode table blk
	
	int blk_buf_id = ext2_read_blk(hook, inode_tbl_blk + cb_offset);
	if(blk_buf_id<0)
		return -1;
	memcpy(hook->inode_buffer, hook->buffer + blk_buf_id * hook->blocksize + offset_in_block, hook->inode_size);
	hook->curr_inode = inode_id;
	return 0;
}

int ext2_write_inode(ext2_hook_t* hook, uint32_t inode_id, ext2_inode_t* data)
{
	if(!hook->valid)
		return -1;
	if(hook->superblock->inode_total <= inode_id)
		return -1;
	
	int blk_grp = (inode_id - 1) / hook->superblock->ins_p_grp; //Containing blockgroup
	int id = (inode_id - 1) % hook->superblock->ins_p_grp;		//Id in blockgroup
	int cb_offset = (id*hook->inode_size) / hook->blocksize;	//offset from first inode table block
	int inodes_per_block = hook->blocksize / hook->inode_size;
	int offset_in_block = (id%inodes_per_block)*hook->inode_size; //offset inside table block
	int inode_tbl_blk = hook->blk_groups[blk_grp].inode_table_blk; //first inode table blk
	
	int blk_buf_id = ext2_read_blk(hook, inode_tbl_blk + cb_offset);
	if(blk_buf_id<0)
		return -1;
	//Copy current content of inode buffer to buffer
	memcpy(hook->buffer + blk_buf_id * hook->blocksize + offset_in_block, data, hook->inode_size);
	//Write data back to device
	ext2_write_blk(hook, inode_tbl_blk + cb_offset, hook->buffer + blk_buf_id * hook->blocksize);
	return 0;
}


int ext2_read_inode_blk(ext2_hook_t* hook, uint32_t inode_id, uint32_t blk_id)
{
	uint32_t blk = ext2_get_inode_blk(hook, inode_id, blk_id);
	
	if(blk==(uint32_t)-1)
		return -1;
	
	return ext2_read_blk(hook, blk);
}

int ext2_write_inode_blk(ext2_hook_t* hook, uint32_t inode_id, uint32_t blk_id, char* data)
{
	uint32_t blk = ext2_get_inode_blk(hook, inode_id, blk_id);
	
	if(blk==(uint32_t)-1)
		return -1;
	
	return ext2_write_blk(hook, blk, data);
}

int ext2_read_inode_content(ext2_hook_t* hook, uint32_t inode_id, uint32_t offset_in_inode, void* target_buf, size_t bytes)
{
	if(ext2_read_inode(hook, inode_id)<0)
		return -1;
	int curr_block = offset_in_inode / hook->blocksize;
	int last_block = (offset_in_inode+bytes) / hook->blocksize;
	int to_read = bytes;
	int offset = offset_in_inode % hook->blocksize;
	for(;curr_block<=last_block && to_read>0; curr_block++)
	{
		int curr_read = hook->blocksize - offset;
		if(to_read < curr_read)
			curr_read = to_read;
		int blk_id = ext2_read_inode_blk(hook, inode_id, curr_block);
		if(blk_id <0)
			return -1;
		memcpy(target_buf, hook->buffer+blk_id*hook->blocksize+offset, curr_read);
		target_buf += curr_read;
		offset=0;
		to_read-=curr_read;
	}
	return 0;
}

int ext2_add_inode_to_dir(ext2_hook_t* hook, uint32_t dir, uint32_t inode, uint8_t type, char* filename)
{
	//Read directory inode, if we can't we failed
	if(ext2_read_inode(hook, dir)<0)
		return -1;
	terminal_writestring("Inode OK\n");
	//If the inode isn't a directory return -1
	if((hook->inode_buffer->type_perm&EXT2_TYPE_DIR) == 0)
		return -1;
	terminal_writestring("Dir OK\n");
	int file_length = strlen(filename);
	if(file_length>EXT2_MAX_FILENAME)
		return -1;
	terminal_writestring("Filename OK\n");
	//Figure out what the last block of the directory is
	int blk_id = (hook->inode_buffer->size_low-1)/hook->blocksize;
	terminal_writeuint32(hook->inode_buffer->size_low);
	
	int blk = ext2_read_inode_blk(hook, dir, blk_id);
	unsigned int buffer_offset = 0;
	ext2_dir_entry_t* curr_entry = 0;
	
	//Check that we won't read outside of the allowed memory
	while(buffer_offset<hook->blocksize)
	{
		curr_entry = (ext2_dir_entry_t*)(hook->buffer+blk*hook->blocksize+buffer_offset);
		//First free entry
		if(curr_entry->total_size+buffer_offset == hook->blocksize)
			break;
		buffer_offset += curr_entry->total_size;
	}
	
	terminal_writeuint32(blk_id);
	terminal_writeuint32(buffer_offset);
	
	//curr_entry is now pointing to the last entry in the directory
	
	//Check if we have enough space to add the folder
	//and if not we grab a new block
	if(curr_entry->total_size < sizeof(ext2_dir_entry_t)+file_length + 3 +curr_entry->name_len)
	{
		//Get new block
		uint32_t new_blk = ext2_get_free_block(hook);
		//Add block to inode
		blk_id = ext2_add_blk_to_inode(hook, inode, new_blk);
		//Read block from inode into buffer
		blk = ext2_read_inode_blk(hook, dir, blk_id);
		//Set current entry to beginning of buffer
		curr_entry = (ext2_dir_entry_t*)(hook->buffer+blk*hook->blocksize);
		//Set size to block size
		curr_entry->total_size = hook->blocksize;
		curr_entry->inode = 0;
		buffer_offset = 0;
	}
	
	int rem_size = curr_entry->total_size;
	if(curr_entry->inode!=0)
	{
		int entry_size = (sizeof(ext2_dir_entry_t) + curr_entry->name_len + 3) & 0xFFF8;
		buffer_offset += entry_size;
		curr_entry->total_size = entry_size;
		rem_size -= entry_size;
		curr_entry = (ext2_dir_entry_t*)(hook->buffer+blk*hook->blocksize+buffer_offset);
	}
	//rem_size contains remaining bytes in block
	curr_entry->inode = inode;
	curr_entry->type = type;
	curr_entry->name_len = file_length;
	curr_entry->total_size = rem_size;
	int new_size = (sizeof(ext2_dir_entry_t)+file_length+3)&0xFFF8;
	//Copy name into correct memory location
	memcpy(curr_entry->name, filename, file_length);
	
	//Write new block to device
	ext2_write_inode_blk(hook, dir, blk_id, hook->buffer+blk*hook->blocksize);
	
	hook->inode_buffer->size_low = blk_id*hook->blocksize+buffer_offset+new_size+sizeof(ext2_dir_entry_t);
	ext2_write_inode(hook, dir, hook->inode_buffer);
	return 0;
}

uint32_t ext2_create_empty_dir(ext2_hook_t* hook, uint32_t dir, char* filename)
{
	uint32_t inode = ext2_get_free_inode(hook);
	uint32_t blk = ext2_get_free_block(hook);
	//If we couldn't get an inode and a block we return -1
	if(inode==(uint32_t)-1 || blk==(uint32_t)-1)
		return -1;
	ext2_mark_inode_used(hook, inode);
	ext2_mark_block_used(hook, blk);
	ext2_add_inode_to_dir(hook, dir, inode, EXT2_FT_DIR, filename);
	
	terminal_writeuint32(blk);
	int blk_buf = ext2_read_blk(hook, blk);
	
	//Set up the two default entries
	ext2_dir_entry_t* entry = (ext2_dir_entry_t*) (hook->buffer + blk_buf*hook->blocksize);
	entry->inode = inode;
	entry->type = EXT2_FT_DIR;
	entry->name_len = 1;
	entry->total_size = 12;
	entry->name[0] = '.';
	entry = (ext2_dir_entry_t*)(hook->buffer + blk_buf*hook->blocksize + entry->total_size);
	entry->inode = dir;
	entry->type = EXT2_FT_DIR;
	entry->name_len = 2;
	entry->total_size = hook->blocksize - 12;
	entry->name[0] = '.';
	entry->name[1] = '.';
	ext2_write_blk(hook, blk, hook->buffer + blk_buf*hook->blocksize);
	
	
	ext2_inode_t new_inode;
	new_inode.type_perm = EXT2_TYPE_DIR; //Directory
	new_inode.size_low = hook->blocksize;
	new_inode.hrd_lnk_count = 0;
	new_inode.sect_used = 0;
	new_inode.size_high = 0;
	new_inode.dbp0 = blk;
	
	//Write the inode to the harddrive
	ext2_write_inode(hook, inode, &new_inode);
	
	ext2_write_info(hook);
	return inode;
}

uint32_t ext2_create_empty_file(ext2_hook_t* hook, uint32_t dir, char* filename)
{
	//Try to get a free inode
	uint32_t inode = ext2_get_free_inode(hook);
	if(inode==(uint32_t)-1)
		return -1;
	
	//If we have one, set up sensible default values for the inode to have
	ext2_inode_t new_inode;
	new_inode.type_perm = EXT2_TYPE_FILE; //File
	new_inode.size_low = 0;
	new_inode.hrd_lnk_count = 0;
	new_inode.sect_used = 0;
	new_inode.size_high = 0;
	
	//Mark the inode used
	ext2_mark_inode_used(hook, inode);
	//Write the inode to the harddrive
	ext2_write_inode(hook, inode, &new_inode);
	//Add the inode to the given directory
	ext2_add_inode_to_dir(hook, dir, inode, EXT2_FT_REG_FILE, filename);
	//Write info of filesystem back
	ext2_write_info(hook);
	return inode;
}

uint32_t ext2_get_free_inode(ext2_hook_t* hook)
{
	//If there are no more free inodes, we fail immediately
	if(!hook->superblock->free_inodes)
		return (uint32_t)-1;
	
	//Go through all block groups and see where we can find a free one
	for(uint32_t x=0; x<hook->num_blk_groups; x++)
	{
		//If there is an unused inode we grab it
		if(hook->blk_groups[x].unused_inodes)
		{
			//Now we need to load the usage bitmap
			int blk_buf_id = ext2_read_blk(hook, hook->blk_groups[x].inode_usage_blk);
			
			//Go through all the possible inodes in here
			char* curr_byte = hook->buffer+(blk_buf_id*hook->blocksize);
			for(uint32_t y=0; y<hook->superblock->ins_p_grp; y++)
			{
				//Current bit is y%8
				int bit = y&0x7;
				//If the current bit is 0 we return its corresponding inode
				if(((1<<bit)&*curr_byte)==0)
					return x*hook->superblock->ins_p_grp+y+1;
				//If we're not on the very first bit and the bit we want to see is 0
				//move to the next byte
				if(y && !bit)
				{
					curr_byte++;
				}
			}
		}
	}
	
	return (uint32_t) -1;
}

int ext2_mark_inode_used(ext2_hook_t* hook, uint32_t inode)
{
	//Figure out what block group the inode is in
	int blk_grp = (inode-1)/hook->superblock->ins_p_grp;
	//Figure out what id the inode has internally
	int internal_id = (inode-1)%hook->superblock->ins_p_grp;
	int byte = internal_id >> 3;
	int bit = internal_id & 0x7;
	
	//Now we need to load the usage bitmap
	int blk_buf_id = ext2_read_blk(hook, hook->blk_groups[blk_grp].inode_usage_blk);
			
	//Get the correct byte for this inode
	char* curr_byte = hook->buffer+(blk_buf_id*hook->blocksize)+byte;
	
	//If the bit wasn't set before we have one less free block now
	if((*curr_byte&(1<<bit))==0)
	{
		hook->blk_groups[blk_grp].unused_inodes--;
		hook->superblock->free_inodes--;
	}
	//Set the correct usage bit
	*curr_byte |= 1<<bit;
	
	//Write back the block
	ext2_write_blk(hook, hook->blk_groups[blk_grp].inode_usage_blk, hook->buffer+(blk_buf_id*hook->blocksize));
	return 0;
}

int ext2_mark_inode_unused(ext2_hook_t* hook, uint32_t inode)
{
	//Figure out what block group the inode is in
	int blk_grp = (inode-1)/hook->superblock->ins_p_grp;
	//Figure out what id the inode has internally
	int internal_id = (inode-1)%hook->superblock->ins_p_grp;
	int byte = internal_id >> 3;
	int bit = internal_id & 0x7;
	
	//Now we need to load the usage bitmap
	int blk_buf_id = ext2_read_blk(hook, hook->blk_groups[blk_grp].inode_usage_blk);
			
	//Get the correct byte for this inode
	char* curr_byte = hook->buffer+(blk_buf_id*hook->blocksize)+byte;
	
	//If the bit was set before we have one more free block now
	if(*curr_byte&(1<<bit))
	{
		hook->blk_groups[blk_grp].unused_inodes++;
		hook->superblock->free_inodes++;
	}
	//unset the correct usage bit
	*curr_byte &= !(1<<bit);
	
	//Write back the block
	ext2_write_blk(hook, hook->blk_groups[blk_grp].inode_usage_blk, hook->buffer+(blk_buf_id*hook->blocksize));
	return 0;
}

uint32_t ext2_get_free_block(ext2_hook_t* hook)
{
	//If there are no more free inodes, we fail immediately
	if(!hook->superblock->free_blocks)
		return (uint32_t)-1;
	
	//Go through all block groups and see where we can find a free one
	for(uint32_t x=0; x<hook->num_blk_groups; x++)
	{
		//If there is an unused inode we grab it
		if(hook->blk_groups[x].unused_blks)
		{
			//Now we need to load the usage bitmap
			int blk_buf_id = ext2_read_blk(hook, hook->blk_groups[x].blk_usage_blk);
			
			//Go through all the possible inodes in here
			char* curr_byte = hook->buffer+(blk_buf_id*hook->blocksize);
			for(uint32_t y=0; y<hook->superblock->blk_p_grp; y++)
			{
				//Current bit is y%8
				int bit = y&0x7;
				//If the current bit is 0 we return its corresponding inode
				if(((1<<bit)&*curr_byte)==0)
					return x*hook->superblock->blk_p_grp+y;
				//If we're not on the very first bit and the bit we want to see is 0
				//move to the next byte
				if(y && !bit)
				{
					curr_byte++;
				}
			}
		}
	}
	
	return (uint32_t) -1;
}

int ext2_mark_block_used(ext2_hook_t* hook, uint32_t blk)
{
	//Figure out what block group the block is in
	int blk_grp = blk/hook->superblock->blk_p_grp;
	//Figure out what id the blk has internally
	int internal_id = blk%hook->superblock->blk_p_grp;
	int byte = internal_id >> 3;
	int bit = internal_id & 0x7;
	
	//Now we need to load the usage bitmap
	int blk_buf_id = ext2_read_blk(hook, hook->blk_groups[blk_grp].blk_usage_blk);
			
	//Get the correct byte for this inode
	char* curr_byte = hook->buffer+(blk_buf_id*hook->blocksize)+byte;
	
	//If the bit wasn't set before we have one less free block now
	if((*curr_byte&(1<<bit))==0)
	{
		hook->blk_groups[blk_grp].unused_blks--;
		hook->superblock->free_blocks--;
	}
	//set the correct usage bit
	*curr_byte &= !(1<<bit);
	
	//Write back the block
	ext2_write_blk(hook, hook->blk_groups[blk_grp].blk_usage_blk, hook->buffer+(blk_buf_id*hook->blocksize));
	return 0;
}

int ext2_mark_block_unused(ext2_hook_t* hook, uint32_t blk)
{
	//Figure out what block group the block is in
	int blk_grp = blk/hook->superblock->blk_p_grp;
	//Figure out what id the blk has internally
	int internal_id = blk%hook->superblock->blk_p_grp;
	int byte = internal_id >> 3;
	int bit = internal_id & 0x7;
	
	//Now we need to load the usage bitmap
	int blk_buf_id = ext2_read_blk(hook, hook->blk_groups[blk_grp].blk_usage_blk);
			
	//Get the correct byte for this inode
	char* curr_byte = hook->buffer+(blk_buf_id*hook->blocksize)+byte;
	
	//If the bit was set before we have one more free block now
	if(*curr_byte&(1<<bit))
	{
		hook->blk_groups[blk_grp].unused_blks++;
		hook->superblock->free_blocks++;
	}
	//unset the correct usage bit
	*curr_byte &= !(1<<bit);
	
	//Write back the block
	ext2_write_blk(hook, hook->blk_groups[blk_grp].blk_usage_blk, hook->buffer+(blk_buf_id*hook->blocksize));
	return 0;
}

uint32_t ext2_add_blk_to_inode(ext2_hook_t* hook, uint32_t inode, uint32_t blk)
{
	if(ext2_read_inode(hook, inode)<0)
		return (uint32_t)-1;
	//Get new free blk_id in inode
	//Guarantee rounding up
	uint32_t blk_id = (hook->inode_buffer->size_low + hook->blocksize-1) / hook->blocksize;
	//Set block reference in inode
	ext2_set_inode_blk(hook, inode, blk_id, blk);
	return blk_id;
}

uint32_t ext2_get_inode_blk(ext2_hook_t* hook, uint32_t inode, uint32_t blk_id)
{
	//Read inode
	if(ext2_read_inode(hook, inode)<0)
		return (uint32_t)-1;
	
	if(blk_id<12)
		return *(&hook->inode_buffer->dbp0+blk_id);
	else if(blk_id < hook->blocksize/4+12)
	{
		int indirect_block = ext2_read_blk(hook, hook->inode_buffer->sibp);
		if(indirect_block < 0)
			return (uint32_t)-1;
		return *((uint32_t*)(hook->buffer + indirect_block * hook->blocksize + (blk_id-12) * 4));
	}
	return (uint32_t)-1;
}

int ext2_set_inode_blk(ext2_hook_t* hook, uint32_t inode, uint32_t blk_id, uint32_t blk)
{
	//Read inode
	if(ext2_read_inode(hook, inode)<0)
		return -1;
	
	if(blk_id<12)
	{
		*(&hook->inode_buffer->dbp0+blk_id) = blk;
	}
	else if(blk_id < hook->blocksize/4+12)
	{
		int indirect_block = ext2_read_blk(hook, hook->inode_buffer->sibp);
		if(indirect_block < 0)
			return (uint32_t)-1;
		*((uint32_t*)(hook->buffer + indirect_block * hook->blocksize + (blk_id-12) * 4)) = blk;
		//Write back to indirect block
		ext2_write_blk(hook, hook->inode_buffer->sibp, hook->buffer + indirect_block * hook->blocksize);
	}
	return 0;
}

int ext2_write_info(ext2_hook_t* hook)
{
	hook->device->write_op(EXT2_SUPERBLOCK_LOC, hook->superblock, EXT2_SUPERBLOCK_SIZE, hook->device->dev_struct);
	hook->device->write_op(EXT2_SUPERBLOCK_LOC+EXT2_SUPERBLOCK_SIZE, hook->blk_groups, hook->blockgroup_blocks*hook->blocksize, hook->device->dev_struct);
	return 0;
}