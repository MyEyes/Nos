#include <format.h>
#include <stdarg.h>
#include <string.h>

char buffer[FORMAT_BUFFER_SIZE];

unsigned long int format_length(const char* sformat, ...)
{
	va_list ap;
	va_start(ap, sformat);
	unsigned long res = _format_length(sformat, &ap);
	va_end(ap);
	return res;
}

unsigned long int _format_length(const char* sformat, va_list* ap)
{
	unsigned long int count = 0;
	
	while(*sformat!=0)
	{
		if(*sformat=='%')
		{
			sformat++;
			count += strlen(format(sformat, ap));
		}
		else
		{
			count++;
		}
		sformat++;
	}	
	return count;
}

int sprintf(char *s, const char *f, ...)
{
	va_list ap;
	va_start(ap, f);
	int res = _sprintf(s,f,&ap);
	va_end(ap);
	return res;
}

int vsprintf(char *s, const char *f, va_list ap)
{
	return _sprintf(s,f,&ap);
}

int _sprintf(char *s, const char *f, va_list* ap)
{
	while(*f)
	{
		if(*f=='%')
		{
			//Skip %
			f++;
			char* form = format(f, ap);
			
			memcpy(s, form, strlen(form));
			s += strlen(form)-1;
		}
		else
		{
			*s = *f;
		}
		f++;
		s++;
	}
	*s=0;
	return 0;
}

char* format(const char* ident, va_list* ap)
{
	switch(*ident)
	{
		case 'd':
		case 'i': return format_decimal(va_arg(*ap, long int));
		case 'u': return format_unsigned_decimal(va_arg(*ap, unsigned long int));
		case 'o': return format_unsigned_octal(va_arg(*ap, unsigned long int));
		case 'p':
		case 'x': return format_lower_hex(va_arg(*ap, unsigned long int));
		case 'X': return format_upper_hex(va_arg(*ap, unsigned long int));
		case 'c': return format_character(va_arg(*ap, unsigned int));
		case 's': return format_string(va_arg(*ap, char*));
	}
	return 0;
}

char* format_decimal(long int num)
{
	char* digits = "0123456789";
	char* curpos = buffer+FORMAT_BUFFER_SIZE-1;
	*curpos-- = 0;
	char sign = num<0;
		
	do{
		*curpos-- = digits[num % 10];
		num /= 10;
	}while(num>0);
	
	if(sign)
		*curpos-- = '-';
	return ++curpos;
}

char* format_unsigned_decimal(unsigned long int num)
{
	char* digits = "0123456789";
	char* curpos = buffer+FORMAT_BUFFER_SIZE-1;
	*curpos-- = 0;
		
	do{
		*curpos-- = digits[num % 10];
		num /= 10;
	}while(num>0);
	
	return ++curpos;
}

char* format_unsigned_octal(unsigned long int num)
{
	char* digits = "01234567";
	char* curpos = buffer+FORMAT_BUFFER_SIZE-1;
	*curpos-- = 0;
		
	do{
		*curpos-- = digits[num % 8];
		num /= 8;
	}while(num>0);
	
	return ++curpos;
}

char* format_lower_hex(unsigned long int num)
{
	char* digits = "0123456789abcdef";
	char* curpos = buffer+FORMAT_BUFFER_SIZE-1;
	*curpos-- = 0;
		
	do{
		*curpos-- = digits[num % 16];
		num /= 16;
	}while(num>0);
	
	return ++curpos;
}

char* format_upper_hex(unsigned long int num)
{
	char* digits = "0123456789ABCDEF";
	char* curpos = buffer+FORMAT_BUFFER_SIZE-1;
	*curpos-- = 0;
		
	do{
		*curpos-- = digits[num % 16];
		num /= 16;
	}while(num>0);
	
	return ++curpos;
}

char* format_character(unsigned int c)
{
	buffer[FORMAT_BUFFER_SIZE-1] = 0;
	buffer[FORMAT_BUFFER_SIZE-2] = (char)c;
	return buffer+FORMAT_BUFFER_SIZE-2;
}

char* format_string(char* s)
{
	return s;
}