DEBUG=false
CFLAGS=-Wall 
CC=gcc
AR=ar

ifeq ($(DEBUG),true)
CFLAGS+=-D_DEBUG -g -Og 
else
CFLAGS+= -O2
endif

ifeq ($(OS),Windows_NT)
CFLAGS+=-D_INC_TCHAR
endif

all: fatfs_dis68k.a

fatfs_dis68k.a: dis68k_io.o ff.o option/cc932.o 
	$(AR) cvq $@ $^

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f fatfs_dis68k.a *.o option/*.o
