include Makefile.dep

CC = $(GCCDIR)gcc
AS = $(GCCDIR)as
LD = $(GCCDIR)ld
STRIP = $(GCCDIR)strip

COFF2NOFF = ../../coff2noff/coff2noff

INCDIR =-I../userprog -I../lib
CFLAGS = -G 0 -O3 -ggdb -c $(INCDIR)

# list of all application sources
SOURCES = add.c halt.c matmult.c shell.c sort.c

# automatically generated lists of intermediary files
OBJS = ${SOURCES:.c=.o}
EXEC = ${SOURCES:.c=}
COFF = ${SOURCES:.c=.coff}
NOFF = ${SOURCES:.c=.noff}

# list of all lib sources to build static libs
# later on  this is the place to add stdarg.c and stdlib.c
LIB_SOURCES =  
LIB_OBJS = ${LIB_SOURCES:.c=.o}

# compile rules
#.SUFFICES: .coff .noff
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

%.coff: %.o
	$(LD) $(LDFLAGS) start.o ${LIB_OBJS} $< -o $@

%.noff: %.coff
	$(STRIP) $<
	$(COFF2NOFF) $< $@

%: %.noff
	./nachos -cp $< $@

# phony targets
.PHONY: all clean distclean copy


all: start.o $(LIB_OBJS) $(COFF2NOFF) $(NOFF)

$(COFF2NOFF):
	Build COFF2NOFF first!

copy: all newdisk $(EXEC)
	./nachos -l

newdisk:
	./nachos -f

clean:
	$(RM) *.o *.ii
	$(RM) *.coff *.noff

distclean: clean
	$(RM) $(EXEC) *~ Makefile.bak
	$(RM) DISK_*

# special targets

start.o: start.s ../userprog/syscall.h
	$(CPP) $(CPPFLAGS) start.s > strt.s
	$(AS) $(ASFLAGS) -o start.o strt.s
	$(RM) strt.s

# next are automatically generated dependencies
# DO NOT DELETE

