#include <elf.h>
#include <terminal.h>
#include <string.h>

task_t* elf_create_proc(elf_header_t* elf_file)
{
	if(elf_file->magic != ELF_MAGIC_WORD)
	{
		terminal_writestring("Not a valid elf image ");
		terminal_writeuint32(elf_file->magic);
		terminal_writestring("\n");
		return (task_t*)0;
	}
	
	if(elf_file->bits != 1)
	{
		terminal_writestring("Can only load 32 bit executables\n");
		return (task_t*)0;
	}
	
	char* elf_raw = (char*)elf_file;
	
	elf_pheader_t* pheader = (elf_pheader_t*)(elf_raw + elf_file->ph_start);
	
	uint32_t start_addr = 0xFFFFFFFF;
	uint32_t end_addr = 0;
	
	for(int x=0; x<elf_file->prog_head_count; x++)
	{
		if(pheader->vaddr<start_addr)
			start_addr=pheader->vaddr;
		if(pheader->vaddr+pheader->memsz>end_addr)
			end_addr=pheader->vaddr+pheader->memsz;
		pheader = (elf_pheader_t*)(((void*)pheader)+elf_file->prog_head_size);
	}
		
	task_t* task = create_user_task((void*)elf_file->entry, (void*)start_addr, (void*)end_addr, 0);
	if(task==0)
	{
		terminal_writestring("Couldn't create task\n");
		return task;
	}
	
	pheader = (elf_pheader_t*)(elf_raw + elf_file->ph_start);
	for(int x=0; x<elf_file->prog_head_count; x++)
	{
		void* tgt_addr = task->ker_mem_start + (pheader->vaddr - start_addr);
		void* src_addr = elf_raw+pheader->offset;
		memcpy(tgt_addr, src_addr, pheader->filesz);
		memzero(tgt_addr+pheader->filesz, pheader->memsz-pheader->filesz);
		pheader = (elf_pheader_t*)(((void*)pheader)+elf_file->prog_head_size);
	}
	return task;
}