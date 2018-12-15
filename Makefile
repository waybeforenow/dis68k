DEBUG=false
LDFLAGS=
CFLAGS=-Wall 

ifeq ($(DEBUG),true)
LDFLAGS+=-g
CFLAGS+=-g -Og
else
CFLAGS+= -O2
endif

ifeq ($(OS),Windows_NT)
CFLAGS+=-D_INC_TCHAR
LDFLAGS+=-liconv
endif

all: dis68k

dis68k: dis68k_main.o dis68k_io.o fatfs/ff.o fatfs/option/cc932.o
	gcc $^ -o $@ $(LDFLAGS)

%.o: %.c
	gcc -c $< -o $@ $(CFLAGS)

clean:
	rm -f dis68k *.o fatfs/*.o fatfs/option/*.o
