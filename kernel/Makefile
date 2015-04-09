
PATH:="~/opt/cross/bin:$(PATH)"

CC = i686-elf-gcc
AS = i686-elf-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c

KERN_FILES = kernel.c res/*.c util/*.c  boot.s util/*.s
KERN_OFILES = *.o

LINKER_FILE = linker.ld
LINKER_PARAMS = -T $(LINKER_FILE)
LINKER_FLAGS = -ffreestanding -O2 -nostdlib

all: kernel_comp nos

kernel_comp: $(KERN_FILES)
	$(CC) $? $(CFLAGS)
	
nos: $(KERN_OFILES)
	$(CC) $(LINKER_PARAMS) -o $@ $(KERN_OFILES) $(LINKER_FLAGS)