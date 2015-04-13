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

.global switch_context_nolevel
switch_context_nolevel:
	
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
	je switch_context_nolevel_no_store
	movl %esp, (%eax)
switch_context_nolevel_no_store:
	movl (target_stack), %eax
	movl %eax, %esp	//Load new stack location from eax into esp
	
	//Copy return address stuff down 8 bytes because
	//iret won't read our ss:esp
	movl 13*4(%esp), %eax
	movl %eax, 15*4(%esp)
	movl 12*4(%esp), %eax
	movl %eax, 14*4(%esp)
	movl 11*4(%esp), %eax
	movl %eax, 13*4(%esp)
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
	
	add $2*4, %esp
	iret

.global no_switch
no_switch:
	push %eax
	movl 3*4(%esp), %eax
	movl %eax, 5*4(%esp)
	movl 2*4(%esp), %eax
	movl %eax, 4*4(%esp)
	movl 1*4(%esp), %eax
	movl %eax, 3*4(%esp)
	pop %eax
	add  $2*4, %esp
	iret
	
.global yield_control
yield_control:
	sub $12, %esp	//offset esp so we can store stuff further up the stack
	push %eax
	push %edx
	push %ecx
	push %ebx
	movl curr_task, %eax
	movl next_task, %edx
	movl $0x0,0xc(%eax)
	movl $0x0,0x10(%eax)
	movl 0x10(%edx),%ebx
	movl 0xc(%edx),%ecx
	movl $0x1,0x8(%eax)
	movl $0x0,0x8(%edx)
	movl %eax,old_task
	movl %edx,curr_task
	movl %eax, 20(%esp)
	movl %edx, 24(%esp)
	//Expected parameters of switch task
	pop %ebx
	pop %ecx
	pop %edx
	pop %eax
	jmp   switch_task

.global switch_task
switch_task:
	//can't regcall because we need to preserve task state
	
	add $4, %esp		//Get rid of return address
	
	//Two parameters on stack
	
	push %eax
	push %edx
	push %ecx
	push %ebx
	//eax is old task
	//edx is new task
	movl 16(%esp), %eax
	movl 20(%esp), %edx
	movl (%eax), %ebx	//Store oldtask->esp in ebx
	movl (%edx), %ecx	//Store newtask->esp in ecx
	
	//Set up memory for switch
	movl %eax, old_stack		//&old_task->esp
	movl %ecx, target_stack		//new_task->esp
	cmp %ebx, %ecx
	jne do_switch		//If they are the same get out of here
	pop %ebx			//Pop values back
	pop %ecx
	pop %edx
	pop %eax
	add $8, %esp		//Remove the two parameters from the stack
	jmp no_switch
do_switch:
	//Determine if we need to switch CPU rings
	movl 4(%eax), %ebx	//Store oldtask->level in ebx
	movl 4(%edx), %ecx	//Store newtask->level in ecx
	and $3, %ebx
	and $3, %ecx		//Only interested in last 2 bits
	cmp %ebx, %ecx		
	jne do_level_switch
	pop %ebx			//Pop values back
	pop %ecx
	pop %edx
	pop %eax
	add $8, %esp		//Remove the two parameters from the stack
	jmp switch_context_nolevel
do_level_switch:
	pop %ebx			//Pop values back
	pop %ecx
	pop %edx
	pop %eax
	add $8, %esp		//Remove the two parameters from the stack
	jmp switch_context

.global call_task
call_task:
	//One parameter on stack
	add $4, %esp		//Get rid of return address
	push %eax
	push %ecx
	
	movl 8(%esp), %eax
	movl (%eax), %ecx	//Store newtask->esp in ecx
	
	//Set up memory for switch
	movl $0, old_stack
	movl %ecx, target_stack		//new_task->esp
	
	pop %ecx
	pop %eax
	add $4, %esp		//Pop values back and rem parameter from stack
	
	jmp switch_context
	