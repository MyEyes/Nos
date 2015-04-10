#ifndef MEM_H
#define MEM_H

#include <stddef.h>

void memcpy(char* trg, char* src, size_t num);
void memzero(char* trg, size_t num);

#endif