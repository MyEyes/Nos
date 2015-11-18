#ifndef _EXT2_H
#define _EXT2_H

#include <stdint.h>
#include <drv/devio.h>

#define EXT2_SIGNATURE 0xef53
#define EXT2_SUPERBLOCK_SIZE 0x400
#define EXT2_SUPERBLOCK_LOC ((void*)0x400)
#define EXT2_BLOCK_BUFFER_SIZE 12

#define EXT2_TYPE_DIR 0x4000
#define EXT2_TYPE_FILE 0x8000

#define EXT2_FT_REG_FILE 1
#define EXT2_FT_DIR 2

#define EXT2_MAX_FILENAME 255

typedef struct
{
	uint32_t inode_total;
	uint32_t block_total;
	uint32_t super_block_count;
	uint32_t free_blocks;
	uint32_t free_inodes;
	uint32_t blk_num;
	uint32_t blk_size;
	uint32_t frg_size;
	uint32_t blk_p_grp;
	uint32_t frg_p_grp;
	uint32_t ins_p_grp;
	uint32_t last_mounted;
	uint32_t last_written;
	uint16_t mnt_count;
	uint16_t max_count;
	uint16_t ext_sig;
	uint16_t state;
	uint16_t err_action;
	uint16_t version_min;
	uint32_t last_chk;
	uint32_t chk_intrv;
	uint32_t crt_os;
	uint32_t version_maj;
	uint16_t res_uid;
	uint16_t res_grpid;
} ext2_superblock_t;

typedef struct
{
	uint32_t first_inode;
	uint16_t inode_size;
	uint16_t blk_group_id;
	uint32_t opt_feat_flags;
	uint32_t req_feat_flags;
	uint32_t wrt_feat_flags;
	char     fs_id[16];
	char	 vol_name[16];
	char	 last_path[64];
	uint32_t compression;
	uint8_t	 prealloc_file;
	uint8_t  prealloc_block;
	uint16_t unused;
	char	 journal_id[16];
	uint32_t journal_inode;
	uint32_t journal_device;
	uint32_t orph_inode_lst;
} ext2_superblock_ext_t;

typedef struct
{
	uint32_t blk_usage_blk;
	uint32_t inode_usage_blk;
	uint32_t inode_table_blk;
	uint16_t unused_blks;
	uint16_t unused_inodes;
	uint16_t dir_count;
	char 	 unused[14];
} ext2_blk_group_desc_t;

typedef struct
{
	uint16_t type_perm; //Type and permission
	uint16_t user_id;
	uint32_t size_low;
	uint32_t last_acc;
	uint32_t creat_time;
	uint32_t last_mod;
	uint32_t del_time;
	uint16_t grp_id;
	uint16_t hrd_lnk_count;
	uint32_t sect_used;
	uint32_t flags;
	uint32_t os_spec_val1;
	//Direct block pointers
	uint32_t dbp0;
	uint32_t dbp1;		
	uint32_t dbp2;		
	uint32_t dbp3;
	uint32_t dbp4;		
	uint32_t dbp5;		
	uint32_t dbp6;		
	uint32_t dbp7;		
	uint32_t dbp8;		
	uint32_t dbp9;		
	uint32_t dbp10;		
	uint32_t dbp11;		
	//Indirect block pointers
	uint32_t sibp; //Singly
	uint32_t dibp; //Doubly
	uint32_t tibp; //Triply
	uint32_t gen_num;
	uint32_t file_acl;
	uint32_t size_high;
	uint32_t frg_blk_addr;
	char	 os_spec_val2[12];
} ext2_inode_t;

typedef struct
{
	uint32_t inode;
	uint16_t total_size;
	uint8_t name_len;
	uint8_t type;
	char	name[];
} ext2_dir_entry_t;

typedef enum
{
	ext2_st_clean=1,
	ext2_st_error=2
} ext2_state;

typedef enum
{
	ext2_ea_ignore 	= 1,
	ext2_ea_remount	= 2,
	ext2_ea_panic	= 3
} ext2_err_act;

typedef enum 
{
	ext2_dir_prealloc = 0x01,
	ext2_afs_serv_exist = 0x02,
	ext2_journal_exist = 0x04,
	ext2_ext_inodes = 0x08,
	ext2_resize = 0x10,
	ext2_dir_hash_id = 0x20
} ext2_opt_feat_flags;

typedef enum
{
	ext2_compression = 0x01,
	ext2_dir_typef = 0x02,
	ext2_repl_journ = 0x04,
	ext2_journ_dev = 0x08
} ext2_req_feat_flags;

typedef enum
{
	ext2_sprs_tbls = 0x01,
	ext2_64b_filesize = 0x02,
	ext2_dir_bin_tree = 0x04
} ext2_wrt_feat_flags;

typedef struct
{
	char valid;
	dev_desc_t* device;
	
	uint16_t next_buffer;
	uint16_t buffer_index[EXT2_BLOCK_BUFFER_SIZE];
	char* buffer;
	
	uint32_t num_blk_groups;
	uint32_t blockgroup_blocks;
	
	uint32_t blocksize;
	uint16_t inode_size;
	
	uint32_t curr_inode;
	ext2_inode_t* inode_buffer;
	
	
	ext2_superblock_t* superblock;
	ext2_superblock_ext_t* superblock_ext;
	ext2_blk_group_desc_t* blk_groups;
} ext2_hook_t;

ext2_hook_t ext2_create_hook(dev_desc_t*);

int ext2_read_inode(ext2_hook_t*, uint32_t);
int ext2_write_inode(ext2_hook_t*, uint32_t, ext2_inode_t*);

int ext2_read_blk(ext2_hook_t*, uint32_t);
int ext2_write_blk(ext2_hook_t*, uint32_t, char*);

int ext2_read_inode_blk(ext2_hook_t*, uint32_t, uint32_t);
int ext2_write_inode_blk(ext2_hook_t*, uint32_t, uint32_t, char*);
int ext2_read_inode_content(ext2_hook_t*, uint32_t, uint32_t, void*, size_t);

int ext2_add_inode_to_dir(ext2_hook_t*, uint32_t, uint32_t, uint8_t, char*);
uint32_t ext2_create_empty_dir(ext2_hook_t*, uint32_t, char*);
uint32_t ext2_create_empty_file(ext2_hook_t*, uint32_t, char*);

uint32_t ext2_add_blk_to_inode(ext2_hook_t*, uint32_t, uint32_t);
uint32_t ext2_get_inode_blk(ext2_hook_t*, uint32_t, uint32_t);
int ext2_set_inode_blk(ext2_hook_t*, uint32_t, uint32_t, uint32_t);

uint32_t ext2_get_free_inode(ext2_hook_t*);
int ext2_mark_inode_used(ext2_hook_t*, uint32_t);
int ext2_mark_inode_unused(ext2_hook_t*, uint32_t);

uint32_t ext2_get_free_block(ext2_hook_t*);
int ext2_mark_block_used(ext2_hook_t*, uint32_t);
int ext2_mark_block_unused(ext2_hook_t*, uint32_t);

int ext2_write_info(ext2_hook_t*);

#endif