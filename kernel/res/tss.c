#include "tss.h"
#include "gdt.h"
#include "mem.h"
#include "../kernel.h"
tss_entry_t* kernel_tss = (tss_entry_t*) KMEM_TSS_LOC;

void init_kernel_tss()
{
	kernel_tss = (tss_entry_t*) KMEM_TSS_LOC;
	memzero((void*)KMEM_TSS_LOC, sizeof(tss_entry_t));
	kernel_tss->ss0 = GDT_KERNEL_DATA_SEG;
	kernel_tss->esp0 = KMEM_SYSCALL_STACK_LOC;
	kernel_tss->iopb_offset = sizeof(tss_entry_t);
}

void load_tss(uint32_t desc)
{
	__asm__ __volatile__(	"movl %%eax, %0 \n\r"
							"ltr %%ax"
							:
							: "aN" (desc)
							:);
}