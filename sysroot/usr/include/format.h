#ifndef _FORMAT_H_
#define _FORMAT_H_

#include <stdarg.h>
#define FORMAT_BUFFER_SIZE 16

#define FORMAT_DISCARD_PARAM(character, ap) 	{																						\
													switch(character)																	\
													{																					\
														case 'd':																		\
														case 'i': va_arg(ap, long int);													\
														case 'u': 																		\
														case 'o':																		\
														case 'p':																		\
														case 'x':																		\
														case 'X': va_arg(ap, unsigned long int);										\
														case 'c': va_arg(ap, unsigned int);												\
														case 's': va_arg(ap, char*);													\
													}																					\
												}
unsigned long format_length(const char*, ...);
unsigned long _format_length(const char*, va_list*);
int _sprintf(char*, const char*, va_list*);
char* format(const char*, va_list*);
char* format_decimal(long int);
char* format_unsigned_decimal(unsigned long int);
char* format_unsigned_octal(unsigned long int);
char* format_lower_hex(unsigned long int);
char* format_upper_hex(unsigned long int);
char* format_character(unsigned int);
char* format_string(char*);
#endif