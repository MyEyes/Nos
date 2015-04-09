#include <stdint.h>
typedef struct
{
	uint64_t base_addr;
	uint64_t reg_length;
	uint32_t reg_type;
	uint32_t acpi_ext;
} meminfo_page_t;

void set_meminfo_ptr(meminfo_page_t** firstPage);
void print_meminfo();