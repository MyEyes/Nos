;----------------------
setDisplayPointer:
	mov ax, 0x0002
	int 0x10
	mov ax, [0x410]		; read bios video descriptor
	and ax, 0x30
	cmp ax, 0x20
	je textmode
	mov ax, [displaycolor]
	jmp colormode
textmode:
	mov ax, [displaytext]   ; text video memory
colormode:
	mov [display], ax
	mov es, ax
	ret

dochar:   call cprint         ; print one character
sprint:   lodsb      ; string char to AL
   cmp al, 0
   jne dochar   ; else, we're done
   add byte [ypos], 1   ;down one row
   mov byte [xpos], 0   ;back to left
   ret
 
cprint:   mov ah, 0x0F   ; attrib = white on black
   mov cx, ax    ; save char/attribute
   
   mov ah, 0
   mov al, byte [xpos];Check if we need to advance to the next line
   cmp ax, word[dwidth]
   jl nonewline
   mov byte [xpos], 0000
   inc byte [ypos]
   call checkscroll
nonewline:
   call checkscroll
   mov dx, 160   ; 2 bytes (char/attrib)
   mul dx      ; for 80 columns
   movzx bx, byte [xpos]
   shl bx, 1    ; times 2 to skip attrib
 
   mov di, 0        ; start of video memory
   add di, ax      ; add y offset
   add di, bx      ; add x offset
 
   mov ax, cx        ; restore char/attribute
   stosw              ; write char/attribute
   add byte [xpos], 1  ; advance to right
   
   ret
   
checkscroll:
   movzx ax, byte [ypos]
   cmp word [dheight], ax;Check that we are still in the right area
   ja noscroll
   call scroll ;if not scroll
   jmp checkscroll ;Loop until done
noscroll:
   mov ax, word [ypos] ;We only come here from after ax being set, so we reset it here
ret
   
scroll:
   mov ax, word [dheight] ;Calculate size of display (without last row)
   ;dec ax
   mov bx, word [dwidth] 	;Width
   mul bx 					;Times height
   
   mov dx, ax				;Mov to dx
   
   mov di, 0 ;Set di to the correct position
   push si	 ;Store si
   push ds
   mov si, [dwidthOffset]
   mov ds, word [display]
   
scrollcopyloop:
   lodsw ;Also increments si
   stosw ;Also increments di
   
   dec dx
   jnz scrollcopyloop
   
   pop ds
   pop si   ;Restore si
   dec byte [ypos]
   mov byte [xpos], 0
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
   mov ax, [reg8]
   mov si, hexstr
   mov cx, 2   ;four places
hexloop8:
   rol ax, 2   ;leftmost will
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
   
xpos   db 0
ypos   db 0
hexstr   db '0123456789ABCDEF'
outstr16   db '0000', 0  ;register value string
reg16   dw    0  ; pass values to printreg16
outstr8 db '00',0
reg8 db 0
displaytext dw 0xa000
displaycolor dw 0xa000
display dw 0
dheight dw 0x0019
dwidth dw 0x0050
dwidthOffset dw 0x00A0