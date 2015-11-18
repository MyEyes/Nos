#ifndef ATA_H_
#define ATA_H_

#include <stdint.h>
#include <stddef.h>

#define ATA_DATA_PORT 0
#define ATA_FEATURE_PORT 1
#define ATA_SECTOR_COUNT_PORT 2
#define ATA_SECTOR_NUMBER_PORT 3
#define ATA_CYLINDER_LOW_PORT 4
#define ATA_CYLINDER_HIGH_PORT 5
#define ATA_DRIVE_HEAD_PORT 6
#define ATA_COMMAND_STATUS_PORT 7

#define ATA_PRIMARY_BUS_PORT 0x1F0
#define ATA_PRIMARY_BUS_DEVICE_CONTROL_PORT 0x3F6

#define ATA_READ_SECTORS_EXT 0x24
#define ATA_WRITE_SECTORS_EXT 0x34
#define ATA_FLUSH_CACHE 0xE7

#define ATA_BLOCK_SIZE 0x200

typedef struct 
{
	int 		status;				//Current status of device
	char		slave;				//master or slave drive on bus
	uint16_t 	base_bus;			//base port of device bus
	uint16_t	device_ctrl_bus;	//device control bus port
	char*		buffer;				//buffer to store sectors
	size_t		buffer_size;
} ata_dev_t;

ata_dev_t* ata_init(uint16_t,uint16_t,char, size_t);
int ata_reset(ata_dev_t*);
int ata_irq_handler(ata_dev_t*);
int ata_send_command(char, ata_dev_t*);
int ata_read(void*, void*, size_t, void*);
int ata_read_to_buf(long unsigned int, size_t, ata_dev_t*);
int ata_copy_to_buf(ata_dev_t*, uint16_t);
int ata_write_from_buf(long unsigned int,size_t, ata_dev_t*);
int ata_copy_from_buf(ata_dev_t*, uint16_t);
int ata_write(void*, void*, size_t, void*);
int ata_select_device(ata_dev_t*);
int ata_wait_busy(ata_dev_t*);

#endif