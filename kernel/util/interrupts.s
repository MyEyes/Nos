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

.global INT40h_handler
INT40h_handler:
	cli
	call int40h
	call acc_interrupt
	sti
	iret
	
.global INT41h_handler
INT41h_handler:
	cli
	call int41h
	call acc_interrupt
	sti
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
	movl (schedule_switch_flag), %eax		//Figure out if we want to switch context
	cmp $0, %eax
	je schedule_no_switch

schedule_no_switch:
	call acc_interrupt
	sti
	iret

page_fault_err: .long 0
.global page_fault_handler
page_fault_handler:
	xchg %bx, %bx
	//We already have an error code on the stack
	movl %eax, page_fault_err
	pop %eax
	xchg page_fault_err, %eax
	call stop_task
	pusha	//8 regs
	pushf	//1 reg
	mov 9*4(%esp), %eax
	push %eax
	mov %cr2, %eax
	push %eax
	call paging_handle_pagefault
	add $8, %esp
	popf
	popa
	add $4, %esp	//Get rid of original error code
	call acc_interrupt
	sti
	iret
	
.global kernel_panic_handler
kernel_panic_handler:
	xchg %bx, %bx
	str 0x1100
	call kernel_panic
	iret
	