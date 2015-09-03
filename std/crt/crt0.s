.section .text
 
.global _start
_start:
	# Set up end of the stack frame linked list.
	mov %eax, (curr_pid)
	mov %ebx, (base_seg)
	mov $0, %ebp
	push %ebp # eip=0
	push %ebp # ebp=0
	mov %esp, %ebp
 
	# We need those in a moment when we call main.
	push %esi
	push %edi
 
	# Prepare signals, memory allocation, stdio and such.
	call std_init
 
	# Run the global constructors.
	#call _init
 
	# Restore argc and argv.
	pop %edi
	pop %esi
 
	# Run main
	call main
	xchg %bx, %bx
	# Terminate the process with the exit code.
	movl %eax, %edi
	push %eax
	push %eax
	jmp exit
	