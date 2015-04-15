#include "pic.h"
#include <stddef.h>
#include "../res/portio.h"
#include "idt.h"

void remap_pic()
{
	unsigned char pm, ps;
	//Store masks
	pm = inb(PIC_MASTER_DAT);
	ps = inb(PIC_SLAVE_DAT);
	wait_io();
	//Starts initialization in cascade mode
	outb(PIC_MASTER_CMD, 0x11);
	outb(PIC_SLAVE_CMD, 0x11);
	wait_io();
	//Set IRQ offsets
	outb(PIC_MASTER_DAT, IRQ_OFFSET);
	outb(PIC_SLAVE_DAT, IRQ_OFFSET+0x08);
	wait_io();
	//Tell master and slave who they are
	outb(PIC_MASTER_DAT, 0b100);
	outb(PIC_SLAVE_DAT,0b010);
	wait_io();
	//Additional info
	outb(PIC_MASTER_DAT, 0x01);
	outb(PIC_SLAVE_DAT, 0x01);
	wait_io();
	//Restore masks
	outb(PIC_MASTER_DAT, pm);
	outb(PIC_SLAVE_DAT, ps);
	wait_io();
}