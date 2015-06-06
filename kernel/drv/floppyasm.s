.global floppy_interrupt
floppy_interrupt:
	xchg %bx, %bx
	xchg %bx, %bx
	cli
	pusha
	call floppy_int_hnd
	popa
	xchg %bx, %bx
	call acc_interrupt
	iret
