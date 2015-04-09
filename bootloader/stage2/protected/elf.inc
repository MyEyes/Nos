elf_load:
	
	;Check for valid magic number
	mov ebx, eax
	mov [elfstart], ebx
	mov ecx, [ebx]
	mov eax, [elf_magic_number]
	cmp eax, ecx
	jne no_elf_error
	
	;Check 32 v 64 bit
	add bx, 4
	
	;Detect endianness
	inc bx
	
	;Detect target OS
	add bx, 2
	
	;Detect elf type
	add bx, 0x09
	
	;Detect arch type
	add bx, 2
	
	;Detect entry point
	add bx, 0x06
	mov ecx, [ebx]
	mov eax, ecx
	mov [kernel_entry], eax
	
	;Find program segments
	add bx, 0x04	;e_phoff
	mov edx, [ebx]
	mov [phoff], edx
	add bx, 0x0E	;e_phentsize
	mov cx, word [ebx]
	mov [phsiz], cx
	add bx, 2		;e_phnum
	mov cx, word [ebx]
	mov [phnum], cx
	
	call elf_ph_load
	
elf_parse_exit:
	ret

no_elf_error:
	jmp elf_parse_exit
	
elf_ph_load:
	;Copy segments into the correct places
	mov dx, [phnum] ;Load number of segments
	and edx, 0xFF
	mov ebx, [elfstart]
	add ebx, [phoff] ;Set bx to correct offset
elf_ph_process:
	mov ecx, [ebx]   	;Read elf  type
	cmp ecx, 1			;If not equal skip elf header
	jne elf_ph_next
	add ebx, 4
	mov esi, [ebx]		;Store offset into file in esi
	add esi, [elfstart]
	add ebx, 4
	mov edi, [ebx]		;Store target vaddr of segment in eax
	add ebx, 8
	push edx
	mov edx, [ebx]		;Store filesz of segment in edx
	cmp edx, 0
	je elf_no_memcpy	;If filesz is 0 we don't need to copy
	call memcpy
elf_no_memcpy:
	mov eax, [ebx]		;Store filesize again
	add ebx, 4
	mov edx, [ebx]		;Store memsize in edx
	sub edx, eax		;Figure out remaining bytes to zero out
	jz elf_no_memzero
	call memzero		;edi still points to the correct byte from memcpy
elf_no_memzero:
	add ebx, 12
	pop edx
	jmp elf_ph_noinc_next
elf_ph_next:
	add bx, word [phsiz]
elf_ph_noinc_next:
	dec dx
	jnz elf_ph_process
	
	ret


elf_magic_number: db 0x7F, "ELF"

elfstart: dd 0
kernel_entry: dd 0
phoff: dd 0
phnum: dw 0
phsiz: dw 0