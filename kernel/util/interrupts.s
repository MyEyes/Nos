.global do_nothing
do_nothing:
	push %eax
	movb $0x20, %al
	outb %al, $0x20
	pop %eax
	iret