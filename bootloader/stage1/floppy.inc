floppy_setup:
	
	mov ah, 8 ;Set up call to detect drive geometry
	mov dl, 0
	int 0x13
	
	and cl, 0x3f
	mov [floppy_noh], dh
	inc byte [floppy_noh]
	mov [floppy_spt], cl
	
	ret
	
floppy_read:
	;expect read address in ax
	;expect target in bx
	mov cl, byte [floppy_spt] 	;Load sectors per track into cl
	mov ch, 0
	mov dx, 0
	div cx						;Figure out temp and sector

	inc dl
	mov byte [floppy_sector], dl
	
	mov cl, byte [floppy_noh]	;Load number of heads into cl
	mov dx, 0
	mov ch, 0
	div cx						;Figure out head and cylinder
	mov byte [floppy_head], dl
	mov word [floppy_cylinder], ax 
	
	jmp floppy_read_retry
	
floppy_read_error:
	push bx
	mov si, floppy_error_msg
	call sprint
	mov  byte [reg8], ah ;look at register
	call printreg8
	call floppy_drive_reset
	pop bx
	
floppy_read_retry:
	mov ax, 0x0201 					;Read 1 sector
	mov cx, word [floppy_cylinder] 	;ch = cylinder & 0xff
	mov ch, cl
	shr cl, 2
	and cx, 0xFFC0 	;cl =sector | ((cylinder>>2) & 0xC0)
	mov ax, word [floppy_sector]
	and ax, 0x003f
	or  cx, ax						;final cx calculation
	mov dh, byte [floppy_head]		;dh=head
									;bx already set to address
	mov dl, 0x00					;Assume drive 0
	mov ax, 0x0201
	int 0x13
	jc floppy_read_error
	ret
	
floppy_drive_reset:
	mov ah, 0
	mov dl, 0
	int 0x13
	ret

floppy_buffer_start: dw stg1_end
floppy_error_msg: db "Error: ", 0
floppy_noh: db 0
floppy_spt: db 0

floppy_sector: db 0
floppy_head: dw 0
floppy_cylinder: dw 0

