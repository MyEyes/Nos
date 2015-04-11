#ifndef PAGING_H
#define PAGING_H

#define PAGE_SIZE 4096
#define PAGE_ENTRY_TO_PTR(ent) ((void*)(((uint32_t)ent)&0xFFFFF000))

typedef enum
{
	PG_Global = 256,
	PG_MB4Page = 128,
	PG_Dirty = 64,
	PG_Accessed = 32,
	PG_CacheDisable = 16,
	PG_WriteTrough = 8,
	PG_User = 4,
	PG_RW = 2,
	PG_Present = 1
} __attribute__((packed)) page_flags;

typedef struct
{
	char* address;
} __attribute__((packed)) page_table_entry_t;

typedef struct
{
	page_table_entry_t entries[1024];
} __attribute__((packed)) page_table_t;

typedef struct
{
	page_table_t* table;
} __attribute__((packed)) page_dir_entry_t;

typedef struct
{
	page_dir_entry_t entries[1024];
} __attribute__((packed)) page_dir_t;

extern page_dir_t* kernel_page_dir;

page_table_t* page_table_create(void* addr);

page_dir_t* page_dir_create(void* addr);

page_dir_entry_t* page_dir_entry_create(page_dir_entry_t* entry, page_table_t* table, page_flags flags);

page_table_entry_t* page_table_entry_create(page_table_entry_t* entry, void* physAddr, page_flags flags);

void enable_paging(page_dir_t* table);

void map_dir(page_dir_t* dir, void* v_addr, void* p_addr);

void map_tbl(page_table_t* tbl, void* v_addr, void* p_addr);

void init_kernel_paging();

#endif