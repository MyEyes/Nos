#ifndef _DMA_H
#define _DMA_H
#include <stdint.h>
#include <stddef.h>

#define DMA_ADDR_REG_CHN1 0x02
#define DMA_CNT_REG_CHN1 0x03
#define DMA_ADDR_REG_CHN2 0x04
#define DMA_CNT_REG_CHN2 0x05
#define DMA_ADDR_REG_CHN3 0x06
#define DMA_CNT_REG_CHN3 0x07
#define DMA_STATUS_SLAVE 0x08
#define DMA_CMD_SLAVE 0x08
#define DMA_REQ_SLAVE 0x09
#define DMA_SNG_MASK_SLAVE 0x0A
#define DMA_MODE_SLAVE 0x0B
#define DMA_FLIPFLOP_SLAVE 0x0C
#define DMA_INTM_SLAVE 0x0D
#define DMA_RST_SLAVE 0x0D
#define DMA_MSK_RST_SLAVE 0x0E
#define DMA_MUL_MASK_SLAVE 0x0F

#define DMA_ADDR_REG_CHN5 0xC4
#define DMA_CNT_REG_CHN5 0xC6
#define DMA_ADDR_REG_CHN6 0xC8
#define DMA_CNT_REG_CHN6 0xCA
#define DMA_ADDR_REG_CHN7 0xCC
#define DMA_CNT_REG_CHN7 0xCE
#define DMA_STATUS_MASTER 0xD0
#define DMA_CMD_MASTER 0xD0
#define DMA_REQ_MASTER 0xD2
#define DMA_SNG_MASK_MASTER 0xD4
#define DMA_MODE_MASTER 0xD6
#define DMA_FLIPFLOP_MASTER 0xD8
#define DMA_INTM_MASTER 0xDA
#define DMA_RST_MASTER 0xDA
#define DMA_MSK_RST_MASTER 0xDC
#define DMA_MUL_MASK_MASTER 0xDE

#define DMA_PG_ADDR_CHN1 0x83
#define DMA_PG_ADDR_CHN2 0x81
#define DMA_PG_ADDR_CHN3 0x82
#define DMA_PG_ADDR_CHN5 0x8B
#define DMA_PG_ADDR_CHN6 0x89
#define DMA_PG_ADDR_CHN7 0x8A

typedef enum
{
	DMA_COUNT_REG,
	DMA_ADDR_REG,
	DMA_PG_ADDR_REG,
	DMA_STATUS_REG,
	DMA_COMMAND_REG,
	DMA_REQ_REG,
	DMA_SNG_MASK_REG,
	DMA_MODE_REG,
	DMA_FLIPFLOP_REG,
	DMA_INTM_REG,
	DMA_RST_REG,
	DMA_MSK_RST_REG,
	DMA_MUL_MASK_REG
} dma_port_type;

typedef enum
{
	DMA_MODE_SEL0 = 1,
	DMA_MODE_SEL1 = 2,
	DMA_MODE_TRA0 = 4,
	DMA_MODE_TRA1 = 8,
	DMA_MODE_AUTO = 16,
	DMA_MODE_DOWN = 32,
	DMA_MODE_MOD0 = 64,
	DMA_MODE_MOD1 = 128,
} dma_mode;

uint8_t dma_get_io_port(dma_port_type, uint8_t);
void dma_mask_chn(uint8_t);
void dma_mask_chn_sng(uint8_t);
void dma_unmask_chn(uint8_t);
void dma_unmask_chn_sng(uint8_t);
void dma_reset_flipflop(uint8_t);
void dma_set_mode(dma_mode, uint8_t);
void dma_map_to_mem(void*, size_t, uint8_t);
void dma_set_addr(void*, uint8_t);
void dma_set_cnt(size_t, uint8_t);
void dma_get_lock();
void dma_release_lock();
#endif