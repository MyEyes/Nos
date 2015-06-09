read_ext2_superblock:
   mov ax, 0x2000
   mov es, ax	;Set segment
   mov ax, 2
   mov bx, 0
   mov cx, 2	;Load sectors 2 and 3 of floppy
   call load_loop
   mov ax, [es:0x0038]
   cmp ax, 0xef53
   je read_ext2_superblock_pass
   mov si, nofs_msg
   call sprint
   jmp hang
read_ext2_superblock_pass:
   mov ax, [es:0x0028]
   mov word [inpergrp], ax
   mov ax, [es:0x001A]
   mov bx, 1024
   mov al, cl
   shl bx, cl
   mov ax, bx
   mov word [blk_size], ax
   shr ax, 9
   mov word [blk_sect], ax
   shl ax, 9
   mov bx, [es:0x0058]
   mov word [inode_size], bx
   mov dx, 0
   div bx	;ax = blk_size/inode_size dx = blk_size%inode_size
   test dx, dx
   jnz hang
   mov word [inperblk], ax
   call read_ext2_block_desc_tbl
ret

read_ext2_block_desc_tbl:
   mov ax, 0x2000
   mov es, ax	;Set segment
   mov ax, 4
   mov bx, 0x400
   mov cx, 1	;Load sector 4 of floppy
   call load_loop
ret

read_ext2_inode:		;AX = inode
   sub ax, 1			;AX = iindex
						;Figure out block group and address of inode tbl
   mov bx, [inpergrp]
   mov dx, 0
   div bx				;AX = iindex/inpergrp = blk_group; DX = iindex % inpergrp
   shl ax, 5			;AX = 32*blk_group = blk_grp_offset
   add ax, 0x408		;AX = &blk_grp_addr->inode_tbl
   mov bx, 0x2000
   mov es, bx
   mov bx, ax
   mov bx, [es:bx]		;BX = blk_grp_addr->inode_tbl
   mov ax, dx			;AX = iindex % inpergrp
   mov dx, 0
   div word [inperblk]	;AX = blk_offset, DX = iindex % inperblock
   push dx
   add ax, bx
   imul ax, [blk_sect]	;AX = floppy_LBA
						;Load inode_tbl from floppy
   mov bx, 0x3000
   mov es, bx
   mov bx, 0
   mov ecx, [blk_sect]
   call load_loop
   
   pop bx				 ;BX = inode_index_in_block
   push bx
   imul bx, [inode_size] ;BX = inode_offset_in_block
   add bx, 4			 ;BX = &node->size
   mov ax, [es:bx]		 ;AX = node->size
   mov dx, 0
   div word [blk_size]		 ;AX = size_in_blocks, DX=0
   mov di, ax
   mov esi, 0
ext2_cpy_loop:   
   cmp si, 12
   je ext2_init_indirect_copy
   ;Get current block pointer
   mov dx, 0x3000
   mov es, dx
   pop bx				 ;BX = inode_index_in_block
   push bx
   imul bx, [inode_size] ;BX = inode_offset_in_block
   add bx, 40			 ;BX = &node->dbp0+si
   mov cx, si			 ;CX = direct block index
   shl cx, 2			 ;CX = direct block index * 4 = direct block offset
   add bx, cx			 ;BX = &node->dbp0+si
   mov ax, [es:bx]		 ;AX = *(&node->dbp0+si)
   imul ax, [blk_sect]   ;AX = *(&node->dbp0+si)*blks_per_sector
   ;Read block from memory
   ;AX is already the correct sector
   mov cx, 0x4000
   mov es, cx
   mov bx, si
   imul bx, [blk_size];BX should be the correct offset
   mov ecx, 0
   mov cx, [blk_sect]
   push si
   push di
   call load_loop
   pop di
   pop si
   
   inc si
   dec di
   jnz ext2_cpy_loop
   
   ;Clean up stack
   pop bx
ret

ext2_init_indirect_copy:
   mov dx, 0x3000
   mov es, dx
   pop bx				 ;BX = inode_index_in_block
   push bx
   imul bx, [inode_size] ;BX = inode_offset_in_block
   add bx, 40			 ;BX = &node->dbp0+si
   mov cx, si			 ;CX = direct block index
   shl cx, 2			 ;CX = direct block index * 4 = direct block offset
   add bx, cx			 ;BX = &node->ibp ;because si=12
   mov ax, [es:bx]		 ;AX = *(&node->dbp0+si)
   imul ax, [blk_sect]   ;AX = *(&node->dbp0+si)*blks_per_sector
   ;Load indirect block to where inode was before
   mov bx, 0
   mov ecx, 0
   mov cx, [blk_sect]
   push si
   push di
   call load_loop
   pop di
   pop si
   pop bx
ext2_indirect_copy_loop:
   
   ;Get current block pointer
   mov dx, 0x3000
   mov es, dx
   mov bx, 0
   mov cx, si			 ;CX = direct block index
   sub cx, 12			 ;Correct si being 12 larger than needed
   shl cx, 2			 ;CX = direct block index * 4 = direct block offset
   add bx, cx			 ;BX = &node->dbp0+si
   mov ax, [es:bx]		 ;AX = *(&node->dbp0+si)
   imul ax, [blk_sect]   ;AX = *(&node->dbp0+si)*blks_per_sector
   ;Read block from memory
   ;AX is already the correct sector
   mov cx, 0x4000
   mov es, cx
   mov bx, si
   imul bx, [blk_size];BX should be the correct offset
   mov ecx, 0
   mov cx, [blk_sect]
   push si
   push di
   call load_loop
   pop di
   pop si
   
   inc si
   dec di
   jnz ext2_indirect_copy_loop
   
ret

nofs_msg db "No FS",10,13,0
fs_found_msg db "EXT2",10,13,0
inpergrp dw 0
inperblk dw 0
blk_size dw 0
blk_sect dw 0
inode_size dw 0