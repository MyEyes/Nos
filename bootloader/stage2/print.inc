
dochar:   call cprint         ; print one character
sprint:   lodsb      ; string char to AL
   cmp al, 0
   jne dochar   ; else, we're done
   ret
 
cprint:   mov ah, 0x0E   ; attrib = white on black
   mov bh, 0
   mov bl, 0x0F
   int 0x10
   ret
   
printreg32:
   mov di, outstr32
   mov eax, [reg32]
   mov si, hexstr
   mov cx, 8   ;eight places
hexloop32:
   rol eax, 4   ;leftmost will
   mov ebx, eax   ; become
   and ebx, 0x0f   ; rightmost
   mov bl, [si + bx];index into hexstr
   mov [di], bl
   inc di
   dec cx
   jnz hexloop32
 
   mov si, outstr32
   call sprint
 
   ret
 
;------------------------------------

printreg16:
   mov di, outstr16
   mov ax, [reg16]
   mov si, hexstr
   mov cx, 4   ;four places
hexloop16:
   rol ax, 4   ;leftmost will
   mov bx, ax   ; become
   and bx, 0x0f   ; rightmost
   mov bl, [si + bx];index into hexstr
   mov [di], bl
   inc di
   dec cx
   jnz hexloop16
 
   mov si, outstr16
   call sprint
 
   ret
 
;------------------------------------

printreg8:
   mov di, outstr8
   mov al, byte [reg8]
   mov si, hexstr
   mov cx, 2   ;two places
hexloop8:
   rol al, 4   ;leftmost will
   mov bx, ax   ; become
   and bx, 0x0f   ; rightmost
   mov bl, [si + bx];index into hexstr
   mov [di], bl
   inc di
   dec cx
   jnz hexloop8
 
   mov si, outstr8
   call sprint
 
   ret
   
print_newline:
	mov si, newline
	call sprint
	ret
   
xpos   db 0
ypos   db 0
hexstr   db '0123456789ABCDEF'

newline: db 10, 13, 0

outstr32   db '00000000 ', 0  ;register value string
reg32   dw    0,0  ; pass values to printreg32

outstr16   db '0000 ', 0  ;register value string
reg16   dw    0  ; pass values to printreg16

outstr8 db '00 ', 0
reg8 db 0

displaytext dw 0xa000
displaycolor dw 0xa000
display dw 0
dheight dw 0x0019
dwidth dw 0x0050
dwidthOffset dw 0x00A0