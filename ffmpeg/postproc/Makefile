
#include config.mak

SWSLIB = libswscale.a

SWSSRCS=swscale.c rgb2rgb.c yuv2rgb.c mp_msg.c aclib.c img_format.c cpudetect.c

SWSOBJS=$(SWSSRCS:.c=.o)

ifeq ($(TARGET_ALTIVEC),yes)
SWSOBJS +=  yuv2rgb_altivec.o
endif

CC=gcc
AR=ar
RANLIB = true
OPTFLAGS = -g -fno-PIC -O2 -pipe -ffast-math -fomit-frame-pointer -D_REENTRANT -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CFLAGS  = $(OPTFLAGS) -I. 

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all:    $(SWSLIB)

$(SWSLIB):     $(SWSOBJS)
	$(AR) r $(SWSLIB) $(SWSOBJS)
	$(RANLIB) $(SWSLIB)

clean:
	rm -f *.o *.a *~ *.soe

distclean: clean
	rm -f .depend

dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SWSSRCS) 1>.depend

#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif

