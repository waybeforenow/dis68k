CC=gcc
CFLAGS=-Wall 
DEBUG=false
PANDOC=pandoc

ifeq ($(DEBUG),true)
	CFLAGS+=-D_DEBUG -g -Og 
else
	CFLAGS+= -O2
endif

ifeq ($(OS),Windows_NT)
	CFLAGS+=-D_INC_TCHAR
endif

.DEFAULT_GOAL := dis68k
.PHONY := all clean doc

all: dis68k doc

clean:
	+$(MAKE) -C lib/fatfs clean
	rm -f dis68k dis68k.1 *.o

doc: dis68k.1

dis68k: dis68k.o dis68k_logging.o dis68k_main.o lib/fatfs/fatfs_dis68k.a
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

lib/fatfs/fatfs_dis68k.a:
	+$(MAKE) -C lib/fatfs fatfs_dis68k.a

dis68k.1: README.md
	$(PANDOC) -s -t man $< -o $@

