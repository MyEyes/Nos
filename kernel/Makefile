
PATH:="~/opt/cross/bin:$(PATH)"

CC = i686-elf-gcc
AS = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c

KERN_C_FILES = kernel.c res/*.c util/*.c int/*.c proc/*.c
KERN_AS_FILES = boot.s int/*.s proc/*s 

KERN_FILES = $(KERN_C_FILES) $(KERN_AS_FILES)
KERN_OFILES = *.o

LINKER_FILE = linker.ld
LINKER_PARAMS = -T $(LINKER_FILE)
LINKER_FLAGS = -ffreestanding -O2 -nostdlib

all: kernel_comp nos

kernel_comp: $(KERN_FILES)
	$(CC) $? $(CFLAGS)
	
nos: $(KERN_OFILES)
	$(CC) $(LINKER_PARAMS) -o $@ $(KERN_OFILES) $(LINKER_FLAGS)