
PATH:="~/opt/cross/bin:$(PATH)"

CC = i686-nos-gcc
AS = i686-nos-as
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -c

STD_C_FILES = ./*.c
STD_AS_FILES = ./*.s

STD_FILES = $(STD_C_FILES) $(STD_AS_FILES)
STD_OFILES = *.o

all: std

std: $(STD_FILES)
	$(CC) $? $(CFLAGS)
	