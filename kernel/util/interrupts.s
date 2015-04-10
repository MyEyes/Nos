.global do_nothing_int
do_nothing_int:
	push %eax
	movb $0x20, %al
	outb %al, $0x20
	pop %eax
	iret

	.global IRQ0_handler
IRQ0_handler:
	push %eax
	push %ebx
	movl (clock_fractions), %eax
	movl (clock_ms), %ebx
	addl %eax, (system_timer_fractions)
	adcl %ebx, (system_timer_ms)		/*Update system clock*/
	
	movb $0x20, %al
	outb %al, $0x20
	pop %ebx
	pop %eax
	iret
	