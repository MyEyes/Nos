#ifndef _EXT2_DS_DRV_H_
#define _EXT2_DS_DRV_H_

#include <drv/file_sys.h>
#include "ext2.h"
#include <drv/devio.h>
#include <stddef.h>

void ext2_fs_init(dev_desc_t);
void ext2_fs_run();

void ext2_fs_getfile(int, fs_cmd_getfile_parm*);
void ext2_fs_readdir(int, fs_cmd_readdir_parm*);
void ext2_fs_readfile(int, fs_cmd_readfile_parm*);
void ext2_fs_writefile(int, fs_cmd_writefile_parm*);
void ext2_fs_createfile(fs_cmd_createfile_parm*);
void ext2_fs_delfile(fs_cmd_delfile_parm*);

#endif