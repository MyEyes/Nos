#ifndef _DEVIO_H
#define _DEVIO_H
#include <stdint.h>
#include <stddef.h>
typedef struct
{
	char 							name[64];
	uint32_t 						op_bm;		//bitmap of supported operations
	int 							(*read_op)(void*, void*, size_t);
	int 							(*write_op)(void*, void*, size_t);
} dev_desc_t;

typedef enum dev_ops
{
	DEV_READ = 0x00,
	DEV_WRITE = 0x01,
};

#endif