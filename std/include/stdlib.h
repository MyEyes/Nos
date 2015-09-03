#ifndef _STDLIB_H
#define _STDLIB_H
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
int atoi(const char*);
void free(void*);
char* getenv(const char*);

//Memory
void* malloc(size_t);
void free(void*);
void* calloc(size_t, size_t);
void* realloc(void*, size_t);

//Environment
extern void exit(int);		//Exists
void abort(void);			//Exists
int atexit(void (*)(void));	//Stub
char* getenv (const char*);	//Stub
int system(const char*);	//Stub
#ifdef __cplusplus
}
#endif
#endif