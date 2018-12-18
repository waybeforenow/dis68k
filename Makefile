DEBUG=false
CFLAGS=-Wall 
CC=gcc

ifeq ($(DEBUG),true)
CFLAGS+=-D_DEBUG -g -Og 
else
CFLAGS+= -O2
endif

ifeq ($(OS),Windows_NT)
CFLAGS+=-D_INC_TCHAR
endif

all: dis68k

dis68k: dis68k.o dis68k_logging.o dis68k_main.o lib/fatfs/fatfs_dis68k.a
	$(CC) $^ -o $@

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

lib/fatfs/fatfs_dis68k.a:
	+$(MAKE) -C lib/fatfs fatfs_dis68k.a

clean:
	+$(MAKE) -C lib/fatfs clean
	rm -f dis68k *.o
