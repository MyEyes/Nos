#include "terminal.h"

uint8_t make_color(enum vga_color fg, enum vga_color bg)
{
	return fg | bg << 4;
}
 
uint16_t make_vgaentry(char c, uint8_t color)
{
	uint16_t c16 = c;
	uint16_t color16 = color;
	return c16 | color16 << 8;
}
 
static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
 
size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer;
 
void terminal_initialize()
{
	terminal_row = 0;
	terminal_column = 0;
	terminal_color = make_color(COLOR_LIGHT_GREY, COLOR_BLACK);
	terminal_buffer = (uint16_t*) 0xB8000;
	for ( size_t y = 0; y < VGA_HEIGHT; y++ )
	{
		for ( size_t x = 0; x < VGA_WIDTH; x++ )
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = make_vgaentry(' ', terminal_color);
		}
	}
}
 
void terminal_setcolor(uint8_t color)
{
	terminal_color = color;
}
 
void terminal_putentryat(char c, uint8_t color, size_t x, size_t y)
{
	if(c<0x20)
		return;
	const size_t index = y * VGA_WIDTH + x;
	terminal_buffer[index] = make_vgaentry(c, color);
}

void scroll_up()
{
	for ( size_t y = 0; y < VGA_HEIGHT-1; y++ )
	{
		for ( size_t x = 0; x < VGA_WIDTH; x++ )
		{
			const size_t index = y * VGA_WIDTH + x;
			terminal_buffer[index] = terminal_buffer[index+VGA_WIDTH];
		}
	}
	const size_t lastLineOffset = (VGA_HEIGHT-1)*VGA_WIDTH;
	for(size_t x=0; x<VGA_WIDTH; x++)
	{
		terminal_buffer[lastLineOffset+x]=make_vgaentry(0, terminal_color);
	}
	terminal_row--;
}
 
void terminal_putchar(char c)
{
	terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
	if ( ++terminal_column == VGA_WIDTH || c==10)
	{
		terminal_column = 0;
		if ( ++terminal_row == VGA_HEIGHT )
		{
			scroll_up();
		}
	}
}
 
void terminal_writestring(const char* data)
{
	size_t datalen = strlen(data);
	for ( size_t i = 0; i < datalen; i++ )
		terminal_putchar(data[i]);
}

void terminal_writebyte(const char c)
{
	const char* buf = "00";
	ctohs(c, buf);	
	terminal_writestring(buf);
}

void terminal_writeuint8(const uint8_t data)
{
	terminal_writebyte((const char) data);
	terminal_writestring(" ");
}

void terminal_writeuint16(const uint16_t data)
{
	char* p = (char*) &data;
	terminal_writebyte(p[1]);
	terminal_writebyte(p[0]);
	terminal_writestring(" ");
}

void terminal_writeuint32(const uint32_t data)
{
	char* p = (char*) &data;
	terminal_writebyte(p[3]);
	terminal_writebyte(p[2]);
	terminal_writebyte(p[1]);
	terminal_writebyte(p[0]);
	terminal_writestring(" ");
}

void terminal_writeuint64(const uint64_t data)
{
	char* p = (char*) &data;
	terminal_writebyte(p[7]);
	terminal_writebyte(p[6]);
	terminal_writebyte(p[5]);
	terminal_writebyte(p[4]);
	terminal_writebyte(p[3]);
	terminal_writebyte(p[2]);
	terminal_writebyte(p[1]);
	terminal_writebyte(p[0]);
	terminal_writestring(" ");
}