floppy_setup:
	mov si, floppy_detect_msg
	call sprint
	
	mov ah, 8 ;Set up call to detect drive geometry
	mov dl, 0
	int 0x13
	
	and cl, 0x3f
	mov [floppy_noh], dh
	inc word [floppy_noh]
	mov [floppy_spt], cl
				
	mov si, floppy_spt_msg
	call sprint
	
	mov dh, [floppy_spt]
	mov byte [reg8], dh
	call printreg8
	
	mov si, floppy_noh_msg
	call sprint
	
	mov dh, [floppy_noh]
	mov byte [reg8], dh
	call printreg8
	
	ret
	
floppy_read:
	;expect read address in ax
	;expect target in bx
	mov cl, byte [floppy_spt] 	;Load sectors per track into cl
	div cl						;Figure out temp and sector

	inc ah
	mov byte [floppy_sector], ah
	and ax, 0x00FF ;Keep only lower bytes of ax
	mov cl, byte [floppy_noh]	;Load number of heads into cl
	div cl						;Figure out head and cylinder
	mov byte [floppy_head], ah
	mov byte [floppy_cylinder], al
	
	jmp floppy_read_retry
	
floppy_read_error:
	push bx
	push ax
	mov si, floppy_error_msg
	call sprint
	pop ax
	mov  byte [reg8], ah ;look at register
	push ax
	call printreg8
	call print_newline
	call floppy_drive_reset
	pop ax
	pop bx
	
	cmp ah, 0x10 ;If we are reading outside of valid disk, exit
	je floppy_read_exit
	
	cmp ah, 0x01 ;If we are reading outside of valid disk, exit
	je floppy_read_exit
	
floppy_read_retry:
	mov ax, word [floppy_cylinder]
	and ax, 0xFF
	mov ch, al 						;ch = cylinder & 0xff
	mov ax, word [floppy_cylinder]
	shr ax, 2
	and ax, 0xC0					;ax = (cylinder>>2) &0xC0
	mov cl, [floppy_sector]			
	or cl, al						;cl |= ax
	mov dh, byte [floppy_head]		;dh=head
									;bx already set to address
	mov dl, 0x00					;Assume drive 0
	mov ax, 0x0201 					;Read 1 sector
	int 0x13
	jc floppy_read_error
floppy_read_exit:
	ret
	
floppy_drive_reset:
	mov ah, 0
	mov dl, 0
	int 0x13
	ret

floppy_detect_msg: db 10,13,"Floppy Check",10,13, 0
floppy_spt_msg: db "SPT: ",0
floppy_noh_msg: db "NOH: ", 0
floppy_error_msg: db "Floppy Error: ", 0
floppy_noh: db 0
floppy_spt: db 0

floppy_sector: db 0
floppy_head: dw 0
floppy_cylinder: dw 0

