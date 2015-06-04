#ifndef PIC_H
#define PIC_H

#define PIC_MASTER_CMD 0x20
#define PIC_MASTER_DAT 0x21
#define PIC_SLAVE_CMD 0xA0
#define PIC_SLAVE_DAT 0xA1

void remap_pic();
#endif