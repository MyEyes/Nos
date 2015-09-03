.global exit
exit:
	call _fini
	pop %eax
	pop %eax
	int $0x41
	