.global jump_usermode
.extern user_mode_entry

jump_usermode:
	movw $0x2B, %ax
	movw %ax, %ds
	movw %ax, %es
	movw %ax, %fs
	movw %ax, %gs
	movl (target_stack), %eax;
	movl %eax, %esp
	//movl %esp, %eax
	push $0x2B
	push %eax		//Push old stack location
	pushf
	push $0x23
	push $user_mode_entry
	iret
	
.global create_kernel_context
create_kernel_context:
	//pop ecx to save return address through stack switch
	pop %ecx
	//Kernel Stack constant defined in kernel.h
	movl $0x6000, %eax
	movl %eax, %esp
	//push ecx to return to correct place
	push %ecx
	ret
	


.global switch_context
switch_context:
	
	//Push state on stack
	push %eax
	push %ecx
	push %edx
	push %ebx
	push %ebp
	push %esi
	push %edi
	push %ds
	push %es
	push %fs
	push %gs
	
	//Somehow needs to store esp for switching back
	movl (old_stack), %eax
	cmp $0, %eax
	je switch_context_no_store
	movl %esp, (%eax)
switch_context_no_store:
	movl (target_stack), %eax
	movl %eax, %esp	//Load new stack location from eax into esp
	
	//Pop state from new stack
	
	pop %gs
	pop %fs
	pop %es
	pop %ds
	pop %edi
	pop %esi
	pop %ebp
	pop %ebx
	pop %edx
	pop %ecx
	pop %eax
	
	iret
	