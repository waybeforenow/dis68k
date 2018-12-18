DEBUG=false
LDFLAGS=
CFLAGS=-Wall 
CC=gcc

ifeq ($(DEBUG),true)
LDFLAGS+=-g
CFLAGS+=-D_DEBUG -g -Og 
else
CFLAGS+= -O2
endif

ifeq ($(OS),Windows_NT)
CFLAGS+=-D_INC_TCHAR
endif

all: dis68k

dis68k: dis68k.o dis68k_io.o dis68k_logging.o dis68k_main.o fatfs/ff.o \
	      fatfs/option/cc932.o
	$(CC) $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(CC) -c $< -o $@ $(CFLAGS)

clean:
	rm -f dis68k *.o fatfs/*.o fatfs/option/*.o
