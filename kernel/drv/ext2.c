#include "ext2.h"
#include <kalloc.h>

ext2_hook_t ext2_create_hook(dev_desc_t* device)
{
	ext2_hook_t result;
	result.device = device;
	result.superblock = (ext2_superblock_t*) kalloc(EXT2_SUPERBLOCK_SIZE);
	result.superblock_ext = (ext2_superblock_ext_t*)(result.superblock + 1);
	device->read_op(EXT2_SUPERBLOCK_LOC, result.superblock, EXT2_SUPERBLOCK_SIZE);
	result.valid = (result.superblock->ext_sig == EXT2_SIGNATURE);
	return result;
}