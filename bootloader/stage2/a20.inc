print_check_a20:
	call check_a20
	cmp ax, 0
	je print_no_a20
	mov si, a20_enabled
	call sprint
print_no_a20:
	sti
	ret

check_a20:
    pushf
    push ds
    push es
    push di
    push si
 
    cli
 
    xor ax, ax ; ax = 0
    mov es, ax
 
    not ax ; ax = 0xFFFF
    mov ds, ax
 
    mov di, 0x0500
    mov si, 0x0510
 
    mov al, byte [es:di]
    push ax
 
    mov al, byte [ds:si]
    push ax
 
    mov byte [es:di], 0x00
    mov byte [ds:si], 0xFF
 
    cmp byte [es:di], 0xFF
 
    pop ax
    mov byte [ds:si], al
 
    pop ax
    mov byte [es:di], al
 
    mov ax, 0
    je check_a20__exit
 
    mov ax, 1
 
check_a20__exit:
    pop si
    pop di
    pop es
    pop ds
    popf
 
    ret
	
set_a20:
	in al, 0x92
	test al, 2
	jnz after
	or al, 2
	and al, 0xFE
	out 0x92, al
after:
	ret
	
a20_enabled: db "A20 is enabled",10,13, 0