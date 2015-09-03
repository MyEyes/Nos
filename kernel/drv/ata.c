#include "ata.h"
#include <kalloc.h>
#include <portio.h>
#include <idt.h>
#include <string.h>

ata_dev_t* selected_dev;

ata_dev_t* ata_init(uint16_t base_port, uint16_t device_ctrl_bus, char slave, size_t buf_size)
{
	ata_dev_t* result = (ata_dev_t*)kalloc(sizeof(ata_dev_t));
	result->base_bus = base_port;
	result->device_ctrl_bus = device_ctrl_bus;
	result->slave = slave;
	result->buffer = kalloc(buf_size);
	result->buffer_size = buf_size;
	set_idt_desc(IRQ_OFFSET+14, (uint32_t)&do_nothing_int, 0, IntGate32, 0x08);
	set_idt_desc(IRQ_OFFSET+15, (uint32_t)&do_nothing_int, 0, IntGate32, 0x08);
	ata_reset(result);
	return result;
}

int ata_reset(ata_dev_t* device)
{
	outb(device->device_ctrl_bus, 0x03);
	outb(device->device_ctrl_bus, 0x01);
	return 0;
}

int ata_send_command(char cmd, ata_dev_t* device)
{
	outb(device->base_bus+ATA_COMMAND_STATUS_PORT, cmd);
	return 0;
}

int ata_select_device(ata_dev_t* device)
{
	if(selected_dev != device)
	{
		outb(device->base_bus+ATA_DRIVE_HEAD_PORT, 0x40+(device->slave?0x10:0));
		//Read status register 5 times to give drive time to respond
		for(int x=0; x<5; x++)
			device->status = inb(device->base_bus+ATA_COMMAND_STATUS_PORT);
		selected_dev = device;
	}
	return 0;
}

int ata_read(void* ladr, void* targ_buf, size_t num_bytes, void* ata_dev)
{
	ata_dev_t* device = (ata_dev_t*) ata_dev;
	long unsigned int lba = (uint32_t)ladr / ATA_BLOCK_SIZE;
	size_t remainder = (uint32_t)ladr - lba * ATA_BLOCK_SIZE;
	size_t num_blocks = (num_bytes+remainder+ATA_BLOCK_SIZE-1)/ATA_BLOCK_SIZE; //Round up
	
	for(unsigned int x=0; x<num_blocks; x++)
	{
		if(ata_read_to_buf(lba+x, 1, device)<0)
			return -1;
		unsigned int cpyBytes = ATA_BLOCK_SIZE-remainder;
		if(num_bytes < cpyBytes)
			cpyBytes = num_bytes;
		memcpy(targ_buf, device->buffer, cpyBytes);
		targ_buf += cpyBytes;
		num_bytes -= cpyBytes;
		remainder = 0;
	}
	return 0;
}

int ata_read_to_buf(long unsigned int lba, size_t num_blocks, ata_dev_t* device)
{
	//Select current drive
	ata_select_device(device);
	
	//Convert address to bytes
	uint8_t lbab[6];
	for(int x=0; x<6; x++)
	{
		lbab[x] = lba & 0xFF;
		lba >>= 8;
	}
	
	//Send number of bytes and address on drive to
	//the respective ports, high bytes first
	outb(device->base_bus + ATA_SECTOR_COUNT_PORT, num_blocks>>8);
	outb(device->base_bus + ATA_SECTOR_NUMBER_PORT, lbab[3]);
	outb(device->base_bus + ATA_CYLINDER_LOW_PORT, lbab[4]);
	outb(device->base_bus + ATA_CYLINDER_HIGH_PORT, lbab[5]);
	outb(device->base_bus + ATA_SECTOR_COUNT_PORT, num_blocks&0xFF);
	outb(device->base_bus + ATA_SECTOR_NUMBER_PORT, lbab[0]);
	outb(device->base_bus + ATA_CYLINDER_LOW_PORT, lbab[1]);
	outb(device->base_bus + ATA_CYLINDER_HIGH_PORT, lbab[2]);
	
	//Send read command
	ata_send_command(ATA_READ_SECTORS_EXT, device);
	
	uint16_t offset = 0;
	do
	{
		if(ata_copy_to_buf(device, offset)<0)
			return -1;
		offset+=256;
		num_blocks-=1;
	}
	while(num_blocks>0);
	
	return 0;
}

int ata_copy_to_buf(ata_dev_t* device, uint16_t offset)
{
	if(ata_wait_busy(device)<0)
		return -1;
	//Otherwise we're good to go
	for(int x=0; x<256; x++)
	{
		((uint16_t*)device->buffer)[offset+x] = inw(device->base_bus+ATA_DATA_PORT);
	}
	
	return 0;
}

int ata_write_from_buf(long unsigned int lba, size_t num_blocks, ata_dev_t* device)
{
	//Select current drive
	ata_select_device(device);
	
	//Convert address to bytes
	uint8_t lbab[6];
	for(int x=0; x<6; x++)
	{
		lbab[x] = lba & 0xFF;
		lba >>= 8;
	}
	//Send number of bytes and address on drive to
	//the respective ports, high bytes first
	outb(device->base_bus + ATA_SECTOR_COUNT_PORT, num_blocks>>8);
	outb(device->base_bus + ATA_SECTOR_NUMBER_PORT, lbab[3]);
	outb(device->base_bus + ATA_CYLINDER_LOW_PORT, lbab[4]);
	outb(device->base_bus + ATA_CYLINDER_HIGH_PORT, lbab[5]);
	outb(device->base_bus + ATA_SECTOR_COUNT_PORT, num_blocks&0xFF);
	outb(device->base_bus + ATA_SECTOR_NUMBER_PORT, lbab[0]);
	outb(device->base_bus + ATA_CYLINDER_LOW_PORT, lbab[1]);
	outb(device->base_bus + ATA_CYLINDER_HIGH_PORT, lbab[2]);
	
	//Send read command
	ata_send_command(ATA_WRITE_SECTORS_EXT, device);
	
	uint16_t offset = 0;
	do
	{
		if(ata_copy_from_buf(device, offset)<0)
			return -1;
		offset+=256;
		num_blocks-=1;
	}
	while(num_blocks>0);
	
	ata_send_command(ATA_FLUSH_CACHE, device);
	//ata_wait_busy(device);
	
	return 0;
}

int ata_copy_from_buf(ata_dev_t* device, uint16_t offset)
{
	if(ata_wait_busy(device)<0)
		return -1;
	//Otherwise we're good to go
	for(int x=0; x<256; x++)
	{
		outw(device->base_bus+ATA_DATA_PORT, ((uint16_t*)device->buffer)[offset+x]);
	}
	
	return 0;
}

int ata_wait_busy(ata_dev_t* device)
{
	//Wait for drive to be ready or error
	char status;
	do
	{
		status = inb(device->base_bus+ATA_COMMAND_STATUS_PORT);
	}
	while((status&0x88) != 0x08 && (status&0x21)==0);
	//We got an error trying to read, so stop right here
	if((status&0x21)!=0)
	{
		return -1;
	}
	return 0;
}