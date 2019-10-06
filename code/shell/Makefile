CC = gcc
LD = gcc
CFLAGS = -g -O2 -DHAVE_CONFIG_H -I.

all: shell

shell: shell.o nachos_syscall.o
	$(LD) $(CFLAGS) -s -o shell shell.o nachos_syscall.o -lc -lrt

clean:
	$(RM) -f shell.o nachos_syscall.o shell

distclean: clean
	$(RM) -f Makefile config.h config.log config.status
