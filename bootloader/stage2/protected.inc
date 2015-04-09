enter_protected:
	cli ; disable interrupts
	lgdt [gdtinfo] ; load global descriptor table
	mov eax, cr0 ; flip first bit of cr0 on
	or al, 1
	mov cr0, eax
	ret
gdtinfo:
	dw gdt_end - gdt -1
	dd gdt
gdt: dd 0,0
kernel_code db 0xff, 0xff, 0, 0, 0x01, 10011010b, 11001111b, 0
kernel_data db 0xff, 0xff, 0, 0, 0x01, 10010010b, 11001111b, 0
gdt_end:
