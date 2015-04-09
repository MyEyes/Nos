detect_ram:
	mov si, RAM_detect_msg
	call sprint
	
	mov di, ram_query_buffer
	mov ebx, 0
ram_detect_next_entry:
	mov edx, 0x534D4150
	mov eax, 0x0000E820
	mov ecx, 24
	int 0x15
	
	jc ram_detect_done ;Check for last entry
	cmp ebx, 0
	je ram_detect_done
	
	push ebx
	push di
	
	mov bx, di
	sub bx, ram_query_buffer
	mov dx, 6
ram_entry_print_loop:
	mov eax, [ram_query_buffer + bx]
	mov dword [reg32], eax
	push bx
	call printreg32
	pop bx
	add bx, 4
	dec dx
	jnz ram_entry_print_loop
	
	call print_newline
	
	pop di
	add di, 24
	pop ebx
	jmp ram_detect_next_entry
ram_detect_done:
	
	ret

av_RAM_msg: db "Available RAM: ", 0
RAM_detect_msg: db "-------------------------RAM-------------------------", 10, 13, 
db "Segment Start     Segment Length    Flags",10,13, 0
ram_query_buffer: times 512 db 0
