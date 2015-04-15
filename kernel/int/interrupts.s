.global acc_interrupt
acc_interrupt:
	push %eax
	movb $0x20, %al
	outb %al, $0x20
	pop %eax
	ret

.global do_nothing_int
do_nothing_int:
	call acc_interrupt
	iret

.global IRQ0_handler
IRQ0_handler:
	push %eax
	push %ebx
	movl (clock_fractions), %eax
	movl (clock_ms), %ebx
	addl %eax, (system_timer_fractions)
	adcl %ebx, (system_timer_ms)		/*Update system clock*/
	
	call acc_interrupt
	pop %ebx
	pop %eax
	iret

	
.global schedule_handler
schedule_handler:
	cli
	
	call stop_task
	
	movl (clock_fractions), %eax
	movl (clock_ms), %ebx
	addl %eax, (system_timer_fractions)
	adcl %ebx, (system_timer_ms)
	addl %eax, (diff_timer_fractions)
	adcl %ebx, (diff_timer_ms)
	
	call schedule

	jmp resume_task

page_fault_err: .long 0
.global page_fault_handler
page_fault_handler:
	//We already have an error code on the stack
	movl %eax, page_fault_err
	pop %eax
	xchg page_fault_err, %eax
	
	call stop_task	//Stop current task, will do nothing if we're still the kernel
	
	pusha	//8 regs
	pushf	//1 reg
	
	//paging_handle_pagefault(cr2, page_fault_err);
	mov page_fault_err, %eax
	push %eax
	mov %cr2, %eax
	push %eax
	call paging_handle_pagefault
	add $8, %esp
	
	popf
	popa
	jmp resume_task
	
.global kernel_panic_handler
kernel_panic_handler:
	xchg %bx, %bx
	str 0x1100
	call kernel_panic
	iret
	