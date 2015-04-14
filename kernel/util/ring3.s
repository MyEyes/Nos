.set KERNEL_STACK, 0x00006000
	
.global create_kernel_context
create_kernel_context:
	//pop ecx to save return address through stack switch
	pop %ecx
	//Kernel Stack constant defined in kernel.h
	movl $KERNEL_STACK, %eax
	movl %eax, %esp
	//push ecx to return to correct place
	push %ecx
	ret


kern_esp: .long 0
ret_addr: .long 0
.global stop_task
stop_task:
	pop ret_addr				//Store return address elsewhere
	sub $4, %esp
	push %eax
	push %esi
	push %edi
	
	
	mov %esp, %eax
	add $4*4, %eax
	mov %eax, 12(%esp)			//Store current esp further up
	mov 5*4(%esp), %eax			//Store cs for iret
	and $3, %eax
	
	jz stop_kern_stop			//If 0 we aren't switching rings
	
	mov 7*4(%esp), %esi			//Store return esp in %esi
	sub $20, %esi				//Move down 5 dwords
	mov %esi, 12(%esp)			//overwrite stored esp with tasks esp
	mov %esp, %edi				//Store %esp in %edi
	add $4*4, %edi				//Move up to where iret header is
	
	mov %esp, %eax
	add $16, %eax
	mov %eax, kern_esp			//Store correct return esp
	
	lodsl 	//1 
	stosl
	lodsl	//2
	stosl
	lodsl	//3
	stosl
	lodsl	//4
	stosl
	lodsl	//5
	stosl
	
	
stop_kern_copy:
	pop %edi
	pop %esi
	pop %eax
	pop %esp					//Now in current tasks stack
	
	push %eax					//Push state on tasks stack
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
	push %esp
	
	mov (curr_task), %eax		//We patch the stack address of the current task
	cmp $0, %eax
	je stop_nopatch
	mov %esp, (%eax)
stop_nopatch:
	mov kern_esp, %eax
	mov %eax, %esp	//Restore kernel stack
	push ret_addr
	ret
	
stop_kern_stop:
	mov %esp, %eax
	sub $8*4, %eax
	mov %eax, kern_esp			//Store correct return esp
	
	jmp stop_kern_copy
	
.global resume_task
resume_task:
	mov (curr_task), %eax		//eax=curr_task
	mov (%eax), %esp			//esp=curr_task->esp
	movl 4(%eax), %eax			//eax=cr3
	mov %eax, %cr3
	pop %esp
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
	call acc_interrupt
	sti
	xchg %bx, %bx
	iret

.global spawn
spawn:
	call stop_task
	movl next_task, %eax
	movl %eax, curr_task
	jmp resume_task
