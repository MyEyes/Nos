#ifndef KEYBOARD_DRV_H_
#define KEYBOARD_DRV_H_
#define KEYBOARD_NUM_KEYS 0x90
#define KEYBOARD_KEY_DOWN 1
#define KEYBOARD_KEY_UP 0

#define KEYBOARD_CMD_GETCHAR 1
#define KEYBOARD_CMD_ISPRESSED 2



typedef struct
{
	unsigned char scantokey[KEYBOARD_NUM_KEYS];
	unsigned char keytoscan[KEYBOARD_NUM_KEYS];
} key_mapping_t;

typedef struct
{
	unsigned char key[KEYBOARD_NUM_KEYS];
} keyboard_state_t;

void keyboard_drv_start();
void keyboard_drv_run();

unsigned char keyboard_issue_cmd(unsigned char);
void keyboard_reset();

unsigned char keyboard_get_char_from_buffer();
void keyboard_add_char_to_buffer(unsigned char);
void keyboard_int_handler();

void keyboard_setkeymap();

void keyboard_read_input();
void keyboard_read_released(unsigned char c);
void keyboard_read_pressed(unsigned char c);
#endif