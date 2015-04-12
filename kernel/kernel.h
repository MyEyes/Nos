#ifndef KERNEL_H
#define KERNEL_H

#define MEMINFO_LOC 0x14000
#define KMEM_PG_DIR_LOC 0x0	//First thing in RAM
#define KMEM_TSS_LOC 0x1000 //4kb right after Page directory
#define KMEM_USER_TSS_LOC 0x1100 //With a bit of free space
#define KMEM_SYSCALL_STACK_LOC 0x6000
#define KMEM_KERN_RESERVED_LOC 0x100000 //1MB
#define KMEM_KERN_RESERVED_SIZE 0x200000 //2MB
#define KMEM_KERN_RESERVED_LIMIT KMEM_KERN_RESERVED_LOC+KMEM_KERN_RESERVED_SIZE //3MB
#define KMEM_PG_TABLE_LOC 0x400000 //4MB
#define KMEM_PG_TABLE_SIZE 0x400000 //4MB
#define KMEM_PG_TABLE_LIMIT KMEM_PG_TABLE_LOC+KMEM_PG_TABLE_SIZE //8MB

#define KMEM_USER_STACK_LOC 0x1000000

extern void *(create_kernel_context)(); //Defined in ring3.s

#endif