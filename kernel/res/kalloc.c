#include "kalloc.h"
#include "paging.h"
#include "meminfo.h"
#include "../kernel.h"
#include "../util/terminal.h"

kalloc_info_t* vmem;

void kalloc_vmem_add(void* addr, uint32_t size)
{
	kalloc_info_t* new = kalloc_create_info(addr, size, KI_free);
	if(vmem)
	{
		kalloc_info_append(kalloc_info_find(vmem, addr), new);
	}
	else
		vmem = kalloc_create_info(addr, size, KI_free);
}

void kalloc_init()
{
	//Map the first few pages for use before we enter proper paging
	for(uint32_t offset = 0; offset<=0x10*PAGE_SIZE; offset+=PAGE_SIZE)
		kernel_map_page((void*)USERSPACE_LOC+offset, (void*)USERSPACE_LOC+offset, KERNEL_PAGE_FLAGS);
	kalloc_vmem_add((void*)USERSPACE_LOC, pmem_total_memory-KMEM_KERNEL_LIMIT);
}

kalloc_info_t* kalloc_vmem_block(void* addr, uint32_t size)
{	
	uint32_t iaddr = ((uint32_t)addr&0xFFFFF000)+PAGE_SIZE-sizeof(kalloc_info_t);
	kalloc_info_t* info = (kalloc_info_t*) iaddr;
	info = kalloc_info_split(kalloc_info_find(vmem, addr), info, size);
	if(info)
		info->state = KI_reserved;
	return info;
}

void kalloc_print_vmem_info()
{
	terminal_writestring("Virt Mem Info:\n");
	terminal_writeuint32((uint32_t)vmem);
	terminal_writestring("\n");
	kalloc_info_t* curr = vmem;
	while(curr)
	{
		kalloc_print_info(curr);
		curr=curr->next;
	}
}

void kalloc_print_info(kalloc_info_t* info)
{
	terminal_writeuint32((uint32_t)info);
	terminal_writestring("\n");
	terminal_writeuint32((uint32_t)info->prev);
	terminal_writeuint32((uint32_t)info->next);
	terminal_writeuint32((uint32_t)info->base_addr);
	terminal_writeuint32((uint32_t)info->size);
	terminal_writeuint8((uint32_t)info->state);
	terminal_writestring("\n");
}

void* kalloc(uint32_t size)
{
	kalloc_info_t* curr = vmem;
	//Add pagesize to hold page info
	size+=PAGE_SIZE;
	//If there are some trailing bytes we round up
	if((size&0xFFF))
	{
		size = (size+PAGE_SIZE)&0xFFFFF000;
	}
	while(curr)
	{
		if(curr->size > size)
		{
			kalloc_info_t* info = kalloc_vmem_block(curr, size);
			if(info)
				return info->base_addr;
		}
		curr=curr->next;
	}
	return 0;
}

kalloc_info_t* kalloc_create_info(void* addr, uint32_t size, kalloc_info_state state)
{
	//Put kalloc info at end of first page of memory region
	uint32_t iaddr = ((uint32_t)addr&0xFFFFF000)+PAGE_SIZE-sizeof(kalloc_info_t);
	kalloc_info_t* info = (kalloc_info_t*) iaddr;
	info->size = (size&0xFFFFF000)-PAGE_SIZE;
	info->state = state;
	info->base_addr = (void*) iaddr+sizeof(kalloc_info_t);
	info->prev=0;
	info->next=0;
	return info;
}

kalloc_info_t* kalloc_info_find(kalloc_info_t* start, void* addr)
{
	kalloc_info_t* curr = start;
	while(curr->base_addr<addr && curr->next)
		curr=curr->next;
	return curr;
}

//size must be 4kb aligned
kalloc_info_t* kalloc_info_split(kalloc_info_t* entry, void* addr, uint32_t size)
{
	if(entry->state == KI_reserved)
		return 0;
	if(size!=(size&0xFFFFF000))
		return 0;
	
	kalloc_info_t* left = entry;
	kalloc_info_t* new = addr;
	kalloc_info_t* right = (void*)((uint32_t)addr+size);
	//Figure out how big the right page will be
	uint32_t rightsize = (((uint32_t)(entry->base_addr)+entry->size)&0xFFFFF000)-(((uint32_t)right)&0xFFFFF000);
	//If there is nothing left on the right set right to entry->next
	if(rightsize)
	{
		//Can only set mem info if we can be sure we won't override something
		right = kalloc_create_info(right, rightsize, entry->state);
	}
	else
	{
		right = entry->next;
	}
	if(right)
	{
		right->prev = new;
		right->next = entry->next;
	}
	//Figure out how big the left page will be
	left->size = ((uint32_t)new&0xFFFFF000)-((uint32_t)left&0xFFFFF000);
	//If the new info takes our memory we make left the prev node
	if(!(left->size))
		left = left->prev;
	if(left)
		left->next = new;
	new=kalloc_create_info(new, size, entry->state);
	new->prev=left;
	new->next=right;
	return new;
}

void kalloc_info_append(kalloc_info_t* old, kalloc_info_t* new)
{
	new->next=old->next;
	new->prev=old;
	old->next=new;
	if(new->next)
	{
		new->next->prev=new;
	}
}

void kalloc_info_delete(kalloc_info_t* entry)
{
	if(entry->prev)
		entry->prev->next=entry->next;
	if(entry->next)
		entry->next->prev=entry->prev;
	
	//Don't need to deallocate here because 
	//entries always reside in their respective memory region
	//BUT we unmap the relevant memory regions
	unmap_dir(kernel_page_dir, (void*)entry);
}

void* kalloc_reqDMA(void* addr, uint32_t size)
{
	void* vAddr = kalloc(size);
	if(vAddr)
	{
		kalloc_info_t* info = vAddr-sizeof(kalloc_info_t);
		info->state |= KI_DMA;
		addr = (void*)(((uint32_t)addr)&0xFFFFF000);
		for(uint32_t offset=0; offset<info->size; offset+=PAGE_SIZE)
			kernel_map_page(vAddr+offset, addr+offset, KERNEL_PAGE_FLAGS|PG_Global);
	}
	return vAddr;
}

void kfree(void* addr)
{
	kalloc_info_t* info = kalloc_info_find(vmem, addr);
	if(info->base_addr!=addr)	//Not an existing entry
		return;
	else if(info->state & KI_reserved)
	{
		for(uint32_t caddr=(uint32_t)info->base_addr;	
		caddr<(uint32_t)info->base_addr+info->size;
		caddr+=PAGE_SIZE)
				unmap_dir(kernel_page_dir, (void*)caddr);
		info->state = KI_free;
		info = kalloc_info_trymerge(info, info->next);
		info = kalloc_info_trymerge(info->prev, info);
	}
}

kalloc_info_t* kalloc_info_trymerge(kalloc_info_t* original, kalloc_info_t* tryout)
{
	//If both pages align
	if(!original)
		return tryout;
	if(!tryout)
		return original;
	if((uint32_t)original->base_addr+original->size == ((uint32_t)tryout&0xFFFFF000))
	{
		if((original->state & KI_free) &&(tryout->state &KI_free))
		{
			original->size += tryout->size + PAGE_SIZE;
			kalloc_info_delete(tryout);
		}
	}
	return original;
}