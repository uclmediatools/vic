# Makefile for tmndec 

# uncomment the following two lines if you want to include X11 support
USE_DISP = -DDISPLAY 
LIBS = -lX11

# uncomment the following two lines if you want to use shared memory
# (faster display if server and client run on the same machine)
USE_SHMEM = -DSH_MEM
LIBS = -lXext -lX11

# uncomment the following line to activate calculation of decoding speed
# (frames per second) and frame rate control (-fn option)
USE_TIME = -DUSE_TIME

# if your X11 include files / libraries are in a non standard location:
# set INCLUDEDIR to -I followed by the appropriate include file path and
# set LIBRARYDIR to -L followed by the appropriate library path and

# Necessary for OpenWindows
INCLUDEDIR = -I/usr/openwin/include
LIBRARYDIR = -L/usr/openwin/lib -Wl,-R,/usr/openwin/lib

# Necessary on some Linux configs
#LIBRARYDIR=-L/usr/X11R6/lib



#
# GNU gcc
#

CC = gcc
#
# This will generate optimized machine code 
#
CFLAGS = -O3 $(DEFS) $(USE_DISP) $(USE_SHMEM) $(USE_TIME) $(INCLUDEDIR)
#
# For debugging and profiling
#CFLAGS = -g -pg -Wall $(DEFS) $(USE_DISP) $(USE_SHMEM) $(USE_TIME) $(INCLUDEDIR)


OBJS = tmndec.o getpic.o getvlc.o gethdr.o getblk.o getbits.o store.o recon.o idct.o idctref.o display.o sac.o dither.o yuv2rgb.o yuvrgb24.o win.o
SRCS = $(subst .o,.c,$(OBJS))


tmndec: $(OBJS)
	$(CC) $(CFLAGS) $(LIBRARYDIR) -o tmndec $(OBJS) -lm $(LIBS)

lib:
	make -f Makefile.lib

win:
	make -f Makefile.win32

clean:
	rm -f *.o *% core *~ TAGS

realclean: clean
	rm -f tmndec tmndec.exe libh263.a


depend dep:
	gccmakedep -- $(CFLAGS) -- $(SRCS)

tar: 
	tar cvf tmndec.tar *.c *.h *.mak Makefile README COPYING CHANGES TODO
	gzip tmndec.tar
	mv -f tmndec.tar.gz tmndec.tgz

# DO NOT DELETE
