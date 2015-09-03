#include <dma.h>
#include <portio.h>
#include <kernel.h>
#include <lock.h>
#include <terminal.h>

lock_t dma_lock;

inline uint8_t dma_get_io_port(dma_port_type type, uint8_t chn)
{
	switch(type)
	{
		case DMA_COUNT_REG:
			switch(chn)
			{
				case 1: return DMA_CNT_REG_CHN1;
				case 2: return DMA_CNT_REG_CHN2;
				case 3: return DMA_CNT_REG_CHN3;
				case 5: return DMA_CNT_REG_CHN5;
				case 6: return DMA_CNT_REG_CHN6;
				case 7: return DMA_CNT_REG_CHN7;
				default: return 0xFF;
			}
		case DMA_ADDR_REG:
			switch(chn)
			{
				case 1: return DMA_ADDR_REG_CHN1;
				case 2: return DMA_ADDR_REG_CHN2;
				case 3: return DMA_ADDR_REG_CHN3;
				case 5: return DMA_ADDR_REG_CHN5;
				case 6: return DMA_ADDR_REG_CHN6;
				case 7: return DMA_ADDR_REG_CHN7;
				default: return 0xFF;
			}
		case DMA_PG_ADDR_REG:
			switch(chn)
			{
				case 1: return DMA_PG_ADDR_CHN1;
				case 2: return DMA_PG_ADDR_CHN2;
				case 3: return DMA_PG_ADDR_CHN3;
				case 5: return DMA_PG_ADDR_CHN5;
				case 6: return DMA_PG_ADDR_CHN6;
				case 7: return DMA_PG_ADDR_CHN7;
				default: return 0xFF;
			}
		default: break;
	}
	if(chn < 4)
	{
		switch(type)
		{
			case DMA_STATUS_REG: return DMA_STATUS_SLAVE;
			case DMA_COMMAND_REG: return DMA_CMD_SLAVE;
			case DMA_REQ_REG: return DMA_REQ_SLAVE;
			case DMA_SNG_MASK_REG: return DMA_SNG_MASK_SLAVE;
			case DMA_MODE_REG: return DMA_MODE_SLAVE;
			case DMA_FLIPFLOP_REG: return DMA_FLIPFLOP_SLAVE;
			case DMA_INTM_REG: return DMA_INTM_SLAVE;
			case DMA_RST_REG: return DMA_RST_SLAVE;
			case DMA_MSK_RST_REG: return DMA_MSK_RST_SLAVE;
			case DMA_MUL_MASK_REG: return DMA_MUL_MASK_SLAVE;
			default: break;
		}
	}
	else if(chn < 8)
	{
		switch(type)
		{
			case DMA_STATUS_REG: return DMA_STATUS_MASTER;
			case DMA_COMMAND_REG: return DMA_CMD_MASTER;
			case DMA_REQ_REG: return DMA_REQ_MASTER;
			case DMA_SNG_MASK_REG: return DMA_SNG_MASK_MASTER;
			case DMA_MODE_REG: return DMA_MODE_MASTER;
			case DMA_FLIPFLOP_REG: return DMA_FLIPFLOP_MASTER;
			case DMA_INTM_REG: return DMA_INTM_MASTER;
			case DMA_RST_REG: return DMA_RST_MASTER;
			case DMA_MSK_RST_REG: return DMA_MSK_RST_MASTER;
			case DMA_MUL_MASK_REG: return DMA_MUL_MASK_MASTER;
			default: break;
		}
	}
	return 0xFF;
}

void dma_reset_flipflop(uint8_t chn)
{
	outb(dma_get_io_port(DMA_FLIPFLOP_REG, chn), 0xFF);
}

void dma_mask_chn(uint8_t chn)
{
	uint8_t port = dma_get_io_port(DMA_MUL_MASK_REG, chn);
	uint8_t mask = inb(port);
	//terminal_writeuint8(port);
	uint8_t mchn = chn & 0x03;
	mchn = 1 << mchn;
	outb(port, mchn|mask);
}

void dma_unmask_chn(uint8_t chn)
{
	uint8_t port = dma_get_io_port(DMA_MUL_MASK_REG, chn);
	uint8_t mask = inb(port);
	uint8_t mchn = chn & 0x03;
	mchn = !(1 << mchn);
	outb(port, mchn&mask);
}

void dma_set_mode(dma_mode mode, uint8_t chn)
{
	uint8_t port = dma_get_io_port(DMA_MODE_REG, chn);
	//Mask Selector out of passed mode and
	//set chn in mode from passed chn value
	mode = mode & 0xFC;
	chn = chn & 0x03;
	mode = mode | chn;
	dma_mask_chn(chn);
	outb(port, mode);
	dma_unmask_chn(chn);
}

void dma_map_to_mem(void* addr, size_t size, uint8_t chn)
{
	uint32_t uiaddr = (uint32_t)addr;
	if(uiaddr < KMEM_DMA_EXCL_LOC || uiaddr + size > KMEM_DMA_EXCL_LIMIT)
		return;
	dma_mask_chn(chn);
	dma_set_addr(addr, chn);
	dma_set_cnt(size, chn);
	dma_set_pg_addr(addr, chn);
	dma_unmask_chn(chn);
}

void dma_set_addr(void* addr, uint8_t chn)
{
	uint32_t uiaddr = (uint32_t)addr;
	if(uiaddr<KMEM_DMA_EXCL_LOC || uiaddr>KMEM_DMA_EXCL_LIMIT)
		return;
	uint8_t addr_low = (uint8_t)uiaddr;
	uiaddr >>= 8;
	uint8_t addr_high = (uint8_t)uiaddr;
	uiaddr >>= 8;
	uint8_t addr_port = dma_get_io_port(DMA_ADDR_REG, chn);
	dma_reset_flipflop(chn);
	outb(addr_port, addr_low);
	outb(addr_port, addr_high);
}

void dma_set_pg_addr(void* addr, uint8_t chn)
{
	uint32_t uiaddr = (uint32_t)addr;
	if(uiaddr<KMEM_DMA_EXCL_LOC || uiaddr>KMEM_DMA_EXCL_LIMIT)
		return;
	uiaddr >>= 16;
	uint8_t addr_pg = (uint8_t)uiaddr;
	uint16_t addr_pg_port = dma_get_io_port(DMA_PG_ADDR_REG, chn);
	outb(addr_pg_port, addr_pg);
}

void dma_set_cnt(size_t size, uint8_t chn)
{
	size--;
	uint8_t size_low = size;
	uint8_t size_high = size>>8;
	uint8_t port = dma_get_io_port(DMA_COUNT_REG, chn);
	dma_reset_flipflop(chn);
	outb(port, size_low);
	outb(port, size_high);
}

void dma_get_lock()
{
	acquire_lock(&dma_lock);
}

void dma_release_lock()
{
	release_lock(&dma_lock);
}