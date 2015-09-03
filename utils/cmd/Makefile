
PATH:="~/opt/cross/bin:$(PATH)"
LIBRARY_PATH:="/usr/lib:$(LIBRARY_PATH)"

CC = i686-nos-gcc
AS = i686-nos-as
AR = i686-nos-ar
LD = i686-nos-ld
CFLAGS = -std=gnu99 -O2 -Wall -Wextra -o $?.o
ARFLAGS = rcs

CMD_C_FILES := $(wildcard *.c)

CMD_FILES := $(CMD_C_FILES)

all: $(CMD_FILES)
	$(CC) $? $(CFLAGS) -o cmd