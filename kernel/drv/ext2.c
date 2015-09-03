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

int ext2_read_inode_blk(ext2_hook_t* hook, uint32_t inode_id, uint32_t blk_id)
{
	if(ext2_read_inode(hook, inode_id)<0)
		return -1;
	if(blk_id<12)
		return ext2_read_blk(hook, *(&hook->inode_buffer->dbp0+blk_id));
	else if(blk_id < hook->blocksize/4+12)
	{
		int indirect_block = ext2_read_blk(hook, hook->inode_buffer->sibp);
		if(indirect_block < 0)
			return -1;
		return ext2_read_blk(hook, *((uint32_t*)(hook->buffer + indirect_block * hook->blocksize + (blk_id-12) * 4)));
	}
	return -1;
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