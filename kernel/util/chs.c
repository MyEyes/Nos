#include <chs.h>

chs_addr_t logical_to_chs(void* addr, chs_info_t info)
{
	uint32_t iaddr = (uint32_t) addr;
	
	chs_addr_t result;
	
	uint32_t temp = iaddr / info.num_sectors;
	result.sector = (iaddr % info.num_sectors) + 1;
	result.head = temp % info.num_heads;
	result.cylinder = temp / info.num_heads;
	
	return result;
}

void* chs_to_logical(chs_addr_t addr, chs_info_t info)
{
	uint32_t iaddr = (addr.cylinder * info.num_heads + addr.head) * info.num_sectors + addr.sector - 1;
	return (void*) iaddr;
}