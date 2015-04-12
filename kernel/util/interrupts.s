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

.global INT40h_handler
INT40h_handler:
	push $0xABABABAB
	push $0xEFEFEFEF
	call int40h
	add $8, %esp
	movb $0x20, %al
	outb %al, $0x20
	iret
	
.global schedule_handler
schedule_handler:
	
	cli
	pusha
	pushf
	
	call schedule
	movl (schedule_switch_flag), %eax		//Figure out if we want to switch context
	cmp $0, %eax
	je schedule_no_switch
	
	movl %esp, %edi							//Store kernel stack pointer in edi
	//pusha stores 8 regs, push f one so we are at 9*4 offset
	//then iret expects eip, cs, flags, esp and ss
	//we want the running processes esp so  12*4
	movl 48(%edi), %edx						//Should be process esp
	movl %edx, %esp							//Set stack pointer to process stack and push iret values
	push 52(%edi)							//Push return ss
	push 48(%edi)							//Push return esp
	push 44(%edi)							//Push return flags
	push 40(%edi)							//Push return cs
	push 36(%edi)							//Push return eip
	movl %esp,   48(%edi)					//Write back esp for int iret with patched esp
	movl $yield_control, %eax
	movl %eax, 36(%edi)						//Make iret return to a yield_control routine that switches context properly				
	movl %edi,%esp
schedule_no_switch:
	movb $0x20, %al
	outb %al, $0x20
	popf
	popa
	sti
	
	iret
	
.global kernel_panic_handler
kernel_panic_handler:
	xchg %bx, %bx
	str 0x1100
	call kernel_panic
	iret
	