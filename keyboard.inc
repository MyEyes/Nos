setupkeyboard:
	cli
	mov bx, 0x09
	shl bx, 2
	xor ax, ax
	mov gs, ax
	mov [gs:bx], word keyhandler
	mov [gs:bx+2], ds
	sti
	ret
keyhandler:
	push ax ;Save registers
	push bx
	
	in al, 0x60
	mov bl, al
	mov byte [port60], al
	
	in al, 0x61
	mov ah, al
	or al, 0x80
	out 0x61, al
	xchg ah, al
	out 0x61, al
	
	mov al, 0x20
	out 0x20, al
	
	and bl, 0x80
	jnz keyhnddone
	
	mov ax, [port60]
	mov word [reg16], ax
	pop bx
	pop ax
	push ax
	push bx
	call printreg16
	
keyhnddone:
	pop bx
	pop ax
	iret
port60 dw 0