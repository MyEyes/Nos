#ifndef _STDIO_H
#define _STDIO_H
#include <stdarg.h>
#include <stddef.h>
#define SEEK_SET 0

#define FILE_TYPE_FILE 0x1
#define FILE_TYPE_DIR 0x2

typedef struct
{
	unsigned int handle;
	unsigned short type;
} FILE;

#ifdef __cplusplus
extern "C" {
#endif

extern FILE* stderr;
#define stderr stderr

int fclose(FILE*);
int fflush(FILE*);
FILE fs_open(FILE dir, const char* path);
FILE* fopen(char*, const char*);
int fprintf(FILE*, const char*, ...);
size_t fread(void*, size_t, size_t, FILE*);
int fseek(FILE*, long, int);
long ftell(FILE*);
size_t fwrite(const void*, size_t, size_t, FILE*);
void setbuf(FILE*, char*);
int vfprintf(FILE*, const char*, va_list);

int set_dir(FILE*);

//Supported
int printf(const char*, ...);
int sprintf(char*, const char*, ...);
int print(const char*);
int puts(const char*);
char getc();

#ifdef __cplusplus
}
#endif

#endif