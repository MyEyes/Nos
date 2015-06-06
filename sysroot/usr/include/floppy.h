#ifndef _FLOPPY_H
#define _FLOPPY_H
#include <stdint.h>
#include <stddef.h>
typedef enum FloppyCommands
{
	READ_TRACK 	=					2,
	SPECIFY	=						3,
	SENSE_DRIVE_STATUS =			4,
	WRITE_DATA =					5,
	READ_DATA =						6,
	RECALIBRATE =					7,
	SENSE_INTERRUPT =				8,
	WRITE_DELETED_DATA =			9,
	READ_ID =						10,
	READ_DELETED_DATA =				12,
	FORMAT_TRACK =					13,
	SEEK =							15,
	VERSION =						16,
	SCAN_EQUAL =					17,
	PERPENDICULAR_MODE =			18,
	CONFIGURE =						19,
	LOCK =							20,
	VERIFY =						22,
	SCAN_LOW_OR_EQUAL =				25,
	SCAN_HIGH_OR_EQUAL =			29,
	OPT_MT =						0x80,
	OPT_MF =						0x40,
	OPT_SK =						0x20
} floppy_cmd;

enum FloppyRegisters
{
	STATUS_REGISTER_A 				= 0x3F0,
	STATUS_REGISTER_B 				= 0x3F1,
	DIGITAL_OUTPUT_REGISTER 		= 0x3F2,
	TAPE_DRIVE_REGISTER				= 0x3F3,
	MAIN_STATUS_REGISTER			= 0x3F4,
	DATARATE_SELECT_REGISTER		= 0x3F4,
	DATA_FIFO						= 0x3F5,
	DIGITAL_INPUT_REGISTER			= 0x3F7,
	CONFIGURATION_CONTROL_REGISTER	= 0x3F7
};

enum DOR_bitflags
{
	MOTD = 1<<7,
	MOTC = 1<<6,
	MOTB = 1<<5,
	MOTA = 1<<4,
	IRQ  = 1<<3,
	RESET= 1<<2,
	DSEL1= 1<<1,
	DSEL0= 1
};

enum MSR_bitflags
{
	RQM 	= 1<<7,
	DIO		= 1<<6,
	NDMA	= 1<<5,
	CB		= 1<<4,
	ACTD	= 1<<3,
	ACTC	= 1<<2,
	ACTB	= 1<<1,
	ACTA	= 1
};

enum CCR_bitflags
{
	MBPS1 = 3,
	KBPS500 = 0
};

enum DIR_bitflags
{
	DISK_CHANGE = 0x80
};

typedef enum
{
	IMPSEEKE = 1<<6,
	FIFOD = 1<<5,
	DPMD = 1<<4
	
} floppy_config_e;

int floppy_init();

int floppy_issue_command(floppy_cmd, uint8_t, char*, uint8_t, char*, char);

int floppy_version();
int floppy_configure(floppy_config_e, char, char);
int floppy_lock();
int floppy_unlock();
int floppy_reset();
int floppy_recalibrate();
int floppy_select_drive(uint8_t drive, char motor_on);

int floppy_check_media_present();
int floppy_read(void*, void*, size_t);
int floppy_write(void*, void*, size_t);

int floppy_setup_dma();
int floppy_set_dma_write();
int floppy_set_dma_read();
#endif