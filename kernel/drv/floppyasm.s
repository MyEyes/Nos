.global floppy_interrupt
floppy_interrupt:
	cli
	pusha
	call floppy_int_hnd
	popa
	call acc_interrupt
	iret
