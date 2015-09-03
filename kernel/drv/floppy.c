#include <floppy.h>
#include <portio.h>
#include <idt.h>
#include <dma.h>
#include <kalloc.h>
#include <clock.h>
#include <string.h>
#include <terminal.h>

volatile char received_IRQ6 = 0;
uint8_t floppy_ver = 0;
char current_drive = 0;

size_t floppy_buf_size;
void* floppy_buf;
const char floppy_dma_channel = 2;

chs_info_t chs_info;

void floppy_int_hnd()
{
	received_IRQ6 = 1;
}

int floppy_issue_command(floppy_cmd cmd, uint8_t argc, char* argv, uint8_t resc, char* resv, char wait_irq)
{
	//Check that drive is ready
	uint8_t status = inb(MAIN_STATUS_REGISTER);
	if((status & (RQM|DIO))!=RQM)
	{
		return -1;
	}
	//Issue command
	outb(DATA_FIFO, cmd);
	//Copy commands, always wait until last byte has been processed
	while(argc-- > 0)
	{
		status = inb(MAIN_STATUS_REGISTER);
		while((status & (RQM|DIO))!=RQM)
		{
			status = inb(MAIN_STATUS_REGISTER);
		}
		outb(DATA_FIFO, *argv++);
	}
	
	//If we need to wait for the IRQ6 we wait here
	while(wait_irq&&!received_IRQ6){}
	if(wait_irq) received_IRQ6 = 0;
	
	if(cmd == RECALIBRATE)
	{
		return 0;
	}
		
	//copy result into resv
	while(resc-- > 0 )
	{
		status = inb(MAIN_STATUS_REGISTER);
		while((status & (RQM|DIO|CB))!=(RQM|DIO|CB))
		{
			status = inb(MAIN_STATUS_REGISTER);
		}
		*resv++ = inb(DATA_FIFO);
	}
	
	status = inb(MAIN_STATUS_REGISTER);
	if((status & (RQM|DIO|CB) ) != RQM)
	{
		return -1;
	}
	return 0;
}

int floppy_version()
{
	char version = 0;
	//Retry 3 times
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(VERSION, 0, (char*)0, 1, &version, 0)>=0)
			return version;
	}
	return -1;
}

int floppy_configure(floppy_config_e flags, char threshhold, char precomp)
{
	char buffer[3];
	buffer[0] = 0;
	buffer[1] = ((char)flags & (threshhold-1));
	buffer[2] = precomp;
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(CONFIGURE, 3, buffer, 0, (char*)0, 0)>=0)
			return 0;
	}
	return -1;
}

int floppy_lock()
{
	char locked = 0;
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(LOCK|OPT_MT, 0, 0, 1, &locked, 0)>=0)
			return locked;
	}
	return -1;
}

int floppy_unlock()
{
	char locked = 0;
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(LOCK, 0, 0, 1, &locked, 0)>=0)
			return locked;
	}
	return -1;
}

int floppy_reset()
{
	outb(DATARATE_SELECT_REGISTER, 0x80);
	
	while(!received_IRQ6);
	received_IRQ6 = 0;
	
	for(int i = 4; i>0; i--)
	{
		floppy_sense_interrupt();
	}
	floppy_select_drive(0, 1);
	return 0;
}

int floppy_specify(char SRT_val, char HUT_val, char HLT_val, char ndma)
{
	char buffer[2];
	buffer[0] = (SRT_val<<4) | HUT_val;
	buffer[1] = (HLT_val<<1) | ndma;
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(SPECIFY, 2, buffer, 0, (char*)0, 0)>=0)
			return 0;
	}
	return -1;
}

int floppy_select_drive(uint8_t drive, char motor_on)
{
	drive &= 0x3f;
	current_drive = drive;
	
	uint8_t drive_motor = 1 << (4+drive);
	outb(CONFIGURATION_CONTROL_REGISTER, 0);
	outb(DIGITAL_OUTPUT_REGISTER, (motor_on?drive_motor:0) | 8 | 4 | drive);
	floppy_specify(8, 5, 0, 0);
	return 0;
}

int floppy_sense_interrupt()
{
	char result[2];
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(SENSE_INTERRUPT, 0, (char*)0, 2, result, 0) >= 0)
			return (result[0]<<8)|result[1];
	}
	return -1;
}

int floppy_recalibrate()
{
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(RECALIBRATE, 1, &current_drive, 0, (char*)0, 1) >= 0)
		{
			int st0 = floppy_sense_interrupt();
			if(st0>0 && st0 & 0x2000)
				return 0;
		}
	}
	return -1;
}

int floppy_read(void* flp_addr, void* buf, size_t num_bytes, void* dev)
{	
	if(num_bytes==0)
		return 0;
	chs_addr_t chs_addr = logical_to_chs(flp_addr, chs_info);
	
	//Figure out how many bytes at the beginning of the sector we should ignore
	uint32_t offset = ((uint32_t)flp_addr) % chs_info.blocksize;
	
	//Figure out how much we want to read into our buffer
	size_t to_read = num_bytes + offset;
	
	if(to_read >= floppy_buf_size)
		to_read = floppy_buf_size;
	
	//Read into floppy buffer
	if(floppy_read_to_buf(chs_addr, to_read)<0)
		return -1;
	
	//terminal_writeuint32((uint32_t)(floppy_buf+offset));
	
	//Copy from floppy buffer to destination
	memcpy(buf, floppy_buf+offset, to_read-offset);
	
	//If we still need to read more, repeat the process
	if(to_read-offset<num_bytes)
		return floppy_read(flp_addr + to_read - offset, buf + to_read - offset, num_bytes - to_read + offset, dev);
	return 0;
}

int floppy_read_to_buf(chs_addr_t chs_addr, size_t num_bytes)
{
	//Figure out how much we want to read into our buffer
	size_t to_read = num_bytes;
	if(to_read >= floppy_buf_size)
		to_read = floppy_buf_size;;
	
	char buffer[8];
	char result[7];
	/*
	terminal_writestring("Reading from: ");
	terminal_writeuint16(chs_addr.head);
	terminal_writeuint16(chs_addr.cylinder);
	terminal_writeuint16(chs_addr.sector);
	terminal_writeuint32(num_bytes);
	terminal_writestring("\n");
	*/
	
	buffer[0] = chs_addr.head<<2 | current_drive;
	buffer[1] = chs_addr.cylinder;
	buffer[2] = chs_addr.head;
	buffer[3] = chs_addr.sector;
	buffer[4] = 2;
	buffer[5] = (num_bytes+chs_info.blocksize-1)/chs_info.blocksize;
	buffer[6] = 0x1b;
	buffer[7] = 0xff;
	
	floppy_set_dma_write();
	for(int i = 4; i>0; --i)
	{
		if(floppy_issue_command(READ_DATA|OPT_MT|OPT_MF, 8, buffer, 7, result, 1)>=0)
		{
			
			/*terminal_writeuint8(result[0]);
			terminal_writeuint8(result[1]);
			terminal_writeuint8(result[2]);
			terminal_writeuint8(result[3]);
			terminal_writeuint8(result[4]);
			terminal_writeuint8(result[5]);
			terminal_writeuint8(result[6]);
			terminal_writestring("\n");
			*/
			
			
			//if there are no errors set in st0
			if((result[0]&0xC0)==0)
			{
				return 0;
			}
		}
	}
	return -1;
}

int floppy_setup_dma()
{
	floppy_buf_size = chs_info.blocksize;
	floppy_buf = kalloc_dma_mem(floppy_buf_size);
	dma_map_to_mem(floppy_buf, floppy_buf_size, floppy_dma_channel);
	return 0;
}

int floppy_set_dma_write()
{
	dma_map_to_mem(floppy_buf, floppy_buf_size, floppy_dma_channel);
	dma_set_mode(DMA_MODE_TRA0|DMA_MODE_MOD0, floppy_dma_channel);
	return 0;
}

int floppy_set_dma_read()
{
	dma_map_to_mem(floppy_buf, floppy_buf_size, floppy_dma_channel);
	dma_set_mode(DMA_MODE_TRA1|DMA_MODE_MOD0, floppy_dma_channel);
	return 0;
}

extern void (*floppy_interrupt)();
extern void (*acc_interrupt)();

int floppy_init()
{
	//Check that we support this drive
	floppy_ver = floppy_version();
	
	if(floppy_ver!=0x90)
		return -2;
	
	chs_info.num_cylinders = 80;
	chs_info.num_heads = 2;
	chs_info.num_sectors = 18;
	chs_info.blocksize = 512;
	
	//Set up IRQ6 interrupt handler
	set_idt_desc(IRQ_OFFSET+6, (uint32_t)&floppy_interrupt, 0, IntGate32, 0x8);
	
	//Configure the drive
	if(floppy_configure(IMPSEEKE|DPMD,8, 0)<0)
		return -1;
	if(floppy_lock()<0)
		return -1;
	
	floppy_reset();
	if(floppy_recalibrate()<0)
		return -1;
	
	floppy_setup_dma();
	
	return 0;
}