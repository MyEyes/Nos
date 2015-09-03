#include <drv/keyboard_drv.h>
#include <kalloc.h>
#include <task.h>
#include <int.h>
#include <idt.h>
#include <ipc/ipc.h>
#include <gdt.h>
#include <debug.h>
#include <unistd.h>
#include <drv_tab.h>
#include <scheduler.h>
#include <portio.h>
#include <stdio.h>

#define KEYBOARD_BUFFER_SIZE 128

task_t* keyboard_task;

char* input_buffer;
int buffer_pos;
int buffer_filled;

char* ipc_buffer;

ipc_msg_hdr_t curr_msg;
ipc_port_t* keyboard_port;

key_mapping_t key_map;
keyboard_state_t keys;

extern void (*keyboard_handler)();

void keyboard_drv_start()
{
	ipc_buffer = kalloc(sizeof(char)*4000);
	input_buffer = kalloc(sizeof(char)*KEYBOARD_BUFFER_SIZE);
	buffer_pos = 0;
	buffer_filled = 0;
	keyboard_task = create_task(keyboard_drv_run, 0, 0, GDT_KERNEL_DATA_SEG, GDT_KERNEL_CODE_SEG, GDT_KERNEL_DATA_SEG, 0);
	keyboard_setkeymap();
	schd_task_add(keyboard_task);
}

void keyboard_drv_run()
{
	keyboard_port = init_port(IPC_PORT_KEYBOARD, 4000);
	open_port(IPC_PORT_KEYBOARD);
	set_idt_desc(IRQ_OFFSET+1, (uint32_t)&keyboard_handler, 0, IntGate32, 0x8);
	//keyboard_reset();
	enable_interrupts();
	//Infinite loop waiting for messages
	while(1)
	{
		//If we got a message we print it
		if(!get_ipc_message(IPC_PORT_KEYBOARD, &curr_msg, ipc_buffer, 4000))
		{
			char c;
			switch(*ipc_buffer)
			{
				case KEYBOARD_CMD_GETCHAR: 
					c = keyboard_get_char_from_buffer();
					send_to_port(curr_msg.reply_port, IPC_PORT_KEYBOARD, &c, 1);
					yield_control_to_port(curr_msg.reply_port);
					break;
				case KEYBOARD_CMD_ISPRESSED: 
					
					break;
			}
		}
		else
		{
			sleep();
		}
	}
}

void keyboard_setkeymap()
{
	key_map.scantokey[0x02] = '1';
	key_map.scantokey[0x03] = '2';
	key_map.scantokey[0x04] = '3';
	key_map.scantokey[0x05] = '4';
	key_map.scantokey[0x06] = '5';
	key_map.scantokey[0x07] = '6';
	key_map.scantokey[0x08] = '7';
	key_map.scantokey[0x09] = '8';
	key_map.scantokey[0x0a] = '9';
	key_map.scantokey[0x0b] = '0';
	key_map.scantokey[0x0c] = 'ß';
	key_map.scantokey[0x0d] = '´';
	key_map.scantokey[0x0e] = '\b'; //Backspace
	key_map.scantokey[0x0f] = '\t'; //Tab
	key_map.scantokey[0x10] = 'q';
	key_map.scantokey[0x11] = 'w';
	key_map.scantokey[0x12] = 'e';
	key_map.scantokey[0x13] = 'r';
	key_map.scantokey[0x14] = 't';
	key_map.scantokey[0x15] = 'z';
	key_map.scantokey[0x16] = 'u';
	key_map.scantokey[0x17] = 'i';
	key_map.scantokey[0x18] = 'o';
	key_map.scantokey[0x19] = 'p';
	key_map.scantokey[0x1a] = 'ü';
	key_map.scantokey[0x1b] = '+';
	key_map.scantokey[0x1c] = '\n';
	key_map.scantokey[0x1e] = 'a';
	key_map.scantokey[0x1f] = 's';
	key_map.scantokey[0x20] = 'd';
	key_map.scantokey[0x21] = 'f';
	key_map.scantokey[0x22] = 'g';
	key_map.scantokey[0x23] = 'h';
	key_map.scantokey[0x24] = 'j';
	key_map.scantokey[0x25] = 'k';
	key_map.scantokey[0x26] = 'l';
	key_map.scantokey[0x27] = 'ö';
	key_map.scantokey[0x28] = 'ä';
	key_map.scantokey[0x29] = '^';
	key_map.scantokey[0x2a] = 15; //Shift
	key_map.scantokey[0x2b] = '#';
	key_map.scantokey[0x2c] = 'y';
	key_map.scantokey[0x2d] = 'x';
	key_map.scantokey[0x2e] = 'c';
	key_map.scantokey[0x2f] = 'v';
	key_map.scantokey[0x30] = 'b';
	key_map.scantokey[0x31] = 'n';
	key_map.scantokey[0x32] = 'm';
	key_map.scantokey[0x33] = ',';
	key_map.scantokey[0x34] = '.';
	key_map.scantokey[0x35] = '-';
	key_map.scantokey[0x36] = 15; //Shift
	key_map.scantokey[0x39] = ' ';
}

void keyboard_int_handler()
{
	keyboard_read_input();
}

void keyboard_read_input()
{
	unsigned char c = inb(0x60);
	if(c>=0x80)
		keyboard_read_released(c);
	else
		keyboard_read_pressed(c);
}

void keyboard_read_released(unsigned char c)
{
	c-=0x80;
	unsigned char character = key_map.scantokey[c];
	keys.key[character] = KEYBOARD_KEY_UP;
}

void keyboard_read_pressed(unsigned char c)
{
	unsigned char character = key_map.scantokey[c];
	keys.key[character] = KEYBOARD_KEY_DOWN;
	//If alphabetical character and shift is pressed
	if(character>=97 && character<=122 && keys.key[15] == KEYBOARD_KEY_DOWN)
		character -= 32;
	keyboard_add_char_to_buffer(character);
}

unsigned char keyboard_issue_command(unsigned char command)
{
	int retries = 3;
	unsigned char result = 0;
	do
	{
		outb(0x60, command);
		result = inb(0x60);
	}
	while(retries-- && result == 0xFE);
	
	return result;
}

void keyboard_reset()
{
	//Repeat reset command until we get the ok
	while(keyboard_issue_command(0xFF)!=0xAA)
		;
	//Tell keyboard to start sending scan codes
	keyboard_issue_command(0xF4);
}

void keyboard_add_char_to_buffer(unsigned char character)
{
	if(buffer_filled>=KEYBOARD_BUFFER_SIZE)
		return;
	if(character!='\n' && character!='\b' && character<32)
		return;
	int position = buffer_pos+buffer_filled;
	if(position>=KEYBOARD_BUFFER_SIZE)
		position -= KEYBOARD_BUFFER_SIZE;
	input_buffer[position] = character;	
	buffer_filled++;
}

unsigned char keyboard_get_char_from_buffer()
{
	if(buffer_filled==0)
		return 0;
	unsigned char result = input_buffer[buffer_pos];
	buffer_pos++;
	if(buffer_pos>=KEYBOARD_BUFFER_SIZE)
		buffer_pos = 0;
	buffer_filled--;
	return result;
}