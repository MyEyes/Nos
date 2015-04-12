#include "tss.h"
#include "gdt.h"
#include "mem.h"
#include "../kernel.h"
#include "../util/terminal.h"
tss_entry_t* kernel_tss = (tss_entry_t*) KMEM_TSS_LOC;

void init_kernel_tss()
{
	kernel_tss = (tss_entry_t*) KMEM_TSS_LOC;
	memzero((void*)KMEM_TSS_LOC, sizeof(tss_entry_t));
	kernel_tss->ss0 = GDT_KERNEL_DATA_SEG;
	kernel_tss->esp0 = KMEM_SYSCALL_STACK_LOC;
	kernel_tss->iopb_offset = sizeof(tss_entry_t);
	
	tss_entry_t* user_tss = (tss_entry_t*) KMEM_USER_TSS_LOC;
	memzero((void*)KMEM_USER_TSS_LOC, sizeof(tss_entry_t));
	user_tss->ss0 = GDT_KERNEL_DATA_SEG; //Add 3 for ring 3 mode
	user_tss->esp0 = KMEM_SYSCALL_STACK_LOC;
	user_tss->iopb_offset = sizeof(tss_entry_t);
}

void load_tss(uint32_t desc)
{
	__asm__ __volatile__(	"movl %%eax, %0 \n\r"
							"ltr %%ax"
							:
							: "aN" (desc)
							:);
}

void print_tss(tss_entry_t* entry)
{
	terminal_writestring("\nTSS: ");
	terminal_writeuint32((uint32_t)entry);
	terminal_writestring("\n");
	terminal_writestring("esp0: ");
	terminal_writeuint32(entry->esp0);
	
	terminal_writestring("ss0 : ");
	terminal_writeuint32(entry->ss0);
	
	terminal_writestring("cr3 : ");
	terminal_writeuint32(entry->cr3);
	terminal_writestring("\n");
	
	terminal_writestring("eip : ");
	terminal_writeuint32(entry->eip);
	
	terminal_writestring("eflg: ");
	terminal_writeuint32(entry->eflags);
	terminal_writestring("\n");
	
	terminal_writestring("eax : ");
	terminal_writeuint32(entry->eax);
	
	terminal_writestring("ebx : ");
	terminal_writeuint32(entry->ebx);
	
	terminal_writestring("ecx : ");
	terminal_writeuint32(entry->ecx);
	terminal_writestring("\n");
	
	terminal_writestring("edx : ");
	terminal_writeuint32(entry->edx);
	
	terminal_writestring("esp : ");
	terminal_writeuint32(entry->esp);
	
	terminal_writestring("ebp : ");
	terminal_writeuint32(entry->ebp);
	terminal_writestring("\n");
	
	terminal_writestring("esi : ");
	terminal_writeuint32(entry->esi);
	
	terminal_writestring("edi : ");
	terminal_writeuint32(entry->edi);
	
	terminal_writestring("es  : ");
	terminal_writeuint32(entry->es);
	terminal_writestring("\n");
	
	terminal_writestring("cs  : ");
	terminal_writeuint32(entry->cs);
	
	terminal_writestring("ss  : ");
	terminal_writeuint32(entry->ss);
	
	terminal_writestring("ds  : ");
	terminal_writeuint32(entry->ds);
	terminal_writestring("\n");
	
	terminal_writestring("fs  : ");
	terminal_writeuint32(entry->fs);
	
	terminal_writestring("gs  : ");
	terminal_writeuint32(entry->gs);
	
	terminal_writestring("ldtr: ");
	terminal_writeuint32(entry->ldtr);
	terminal_writestring("\n");
}