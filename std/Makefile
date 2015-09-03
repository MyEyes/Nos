
PATH:="~/opt/cross/bin:$(PATH)"

CC = i686-nos-gcc
AS = i686-nos-as
AR = i686-nos-ar
CFLAGS = -std=gnu99 -O2 -Wall -Wextra -c
ARFLAGS = rcs

STD_C_FILES := $(wildcard *.c ipc/*.c)
STD_AS_FILES := ./*.s
STD_CRT_FILES := crt/crt0.s crt/crti.s crt/crtn.s

STD_FILES := $(STD_C_FILES) $(STD_AS_FILES)

CRT_OFILES = crtbegin.o crtend.o
STD_OFILES_T := $(wildcard *.o)
STD_OFILES := $(filter-out $(CRT_OFILES),$(STD_OFILES_T))

TEST_FILES = $(wildcard test/*.c)

all: std std_crt std_link

std: $(STD_FILES)
	$(CC) $? $(CFLAGS)
	
std_crt: $(STD_CRT_FILES)
	$(CC) $? $(CFLAGS)
	
std_link: $(STD_OFILES)
	rm libc.a
	$(AR) $(ARFLAGS) libc.a $?

test: $(TEST_FILES)
	$(CC) $? -std=gnu99 -Wall -Wextra -o $?.o
	
install:
	cp libc.a ../sysroot/usr/lib/libc.a
	cp crt0.o ../sysroot/usr/lib/crt0.o
	cp crti.o ../sysroot/usr/lib/crti.o
	cp crtn.o ../sysroot/usr/lib/crtn.o
	cp libc.a /usr/lib/cross/libc.a
	cp crt0.o /usr/lib/cross/crt0.o
	cp crti.o /usr/lib/cross/crti.o
	cp crtn.o /usr/lib/cross/crtn.o
test-install:
	cp test/stdtest.c.o /cygdrive/f/stdtest.c.o