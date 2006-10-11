
include config.mak

SWSLIB = libswscale.a

SWSSRCS=swscale.c rgb2rgb.c yuv2rgb.c mp_msg.c aclib.c img_format.c

SWSOBJS=$(SWSSRCS:.c=.o)

ifeq ($(TARGET_ALTIVEC),yes)
SWSOBJS +=  yuv2rgb_altivec.o
endif

CFLAGS  = $(OPTFLAGS) $(MLIB_INC) -I. $(EXTRA_INC)
# -I/usr/X11R6/include/

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -I.. -o $@ $<

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

