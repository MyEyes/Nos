.global exit
exit:
	push %eax
	call _fini
	pop %eax
	int $0x41
	