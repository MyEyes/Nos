#ifndef PORTIO_H
#define PORTIO_H

#include <stdint.h>

void outb(uint16_t port, uint8_t val);
void outw(uint16_t port, uint16_t val);
void outdw(uint16_t port, uint32_t val);

uint8_t inb(uint16_t port);
uint16_t inw(uint16_t port);
uint32_t indw(uint16_t port);

void wait_io();
#endif