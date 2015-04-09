memcpy:
mem_cpy_loop:
	lodsb
	stosb
	dec edx
	jnz mem_cpy_loop
	ret
	
memzero:
	mov al, 0
mem_zero_loop:
	stosb
	dec edx
	jnz mem_zero_loop
	ret
