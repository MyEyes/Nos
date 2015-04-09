elf_load:
	push ax
	mov si, elf_parse_msg
	call sprint

	mov [reg16], es
	call printreg16
	
	pop ax
	mov [reg16], ax
	push ax
	call printreg16
	
	call print_newline
	
	pop ax
	
	;Check for valid magic number
	mov bx, ax
	mov [elfstart], bx
	mov ecx, [es:bx]
	mov eax, [elf_magic_number]
	cmp eax, ecx
	jne no_elf_error
	
	;Check 32 v 64 bit
	add bx, 4
	mov cl, [es:bx]
	mov si, bit64_msg
	cmp cl, 1
	jne detect_64_bit
	mov si, bit32_msg
detect_64_bit:
	push bx
	call sprint
	pop bx
	
	;Detect endianness
	inc bx
	mov cl, [es:bx]
	mov si, be_msg
	cmp cl, 1
	jne detect_big_endian
	mov si, le_msg
detect_big_endian:
	push bx
	call sprint
	pop bx
	
	;Detect target OS
	add bx, 2
	mov cl, [es:bx]
	push bx
	mov si, os_msg
	call sprint
	mov [reg8], cl
	call printreg8
	call print_newline
	pop bx
	
	;Detect elf type
	add bx, 0x09
	mov cx, [es:bx]
	push bx
	mov si, type_msg
	call sprint
	mov [reg16], cx
	call printreg16
	call print_newline
	pop bx
	
	;Detect arch type
	add bx, 2
	mov cx, [es:bx]
	push bx
	mov si, arch_msg
	call sprint
	mov [reg16], cx
	call printreg16
	call print_newline
	pop bx
	
	;Detect entry point
	add bx, 0x06
	mov ecx, [es:bx]
	mov eax, ecx
	mov [kernel_entry], eax
	push eax
	push bx
	mov si, entry_msg
	call sprint
	mov [reg32], ecx
	call printreg32
	call print_newline
	pop bx
	pop eax
	
	;Find program segments
	add bx, 0x04	;e_phoff
	mov ecx, [es:bx]
	mov [phoff], ecx
	add bx, 0x0C	;e_phentsize
	mov cx, [es:bx]
	mov [phsiz], cx
	add bx, 2		;e_phnum
	mov cx, [es:bx]
	mov [phnum], cx
	
elf_parse_exit:
	push eax
	call print_newline
	call print_newline
	pop eax
	ret


no_elf_error:
	mov si, no_elf_error_msg
	call sprint
	jmp elf_parse_exit
	
load_elf:
	;Copy segments into the correct places
	mov dx, [phnum] ;Load number of segments
	and edx, 0xFF
	mov bx, [elfstart]
	add bx, [phoff] ;Set bx to correct offset
elf_ph_process:
	mov ecx, [es:bx]
elf_ph_next:
	dec dx
	jnz elf_ph_process
	jmp load_elf


elf_magic_number: db 0x7F, "ELF"
no_elf_error_msg: db "Invalid elf file"
elf_parse_msg: db 10,13,"Parsing elf file at: ", 0
bit32_msg: db "32 bit",10,13, 0
bit64_msg: db "64 bit",10,13, 0
le_msg: db "little endian",10,13, 0
be_msg: db "big endian",10,13, 0
os_msg: db "Target OS:   ", 0
type_msg: db "Elf Type:    ", 0
arch_msg: db "Target Arch: ",0
entry_msg: db "Entry Point: ", 0
elfstart: dw 0

phoff: dd 0
phnum: dw 0
phsiz: dw 0