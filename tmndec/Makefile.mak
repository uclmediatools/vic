# Makefile for tmndec 

# This is free software. See the file COPYING for copying conditions.

NODEBUG=1
TARGETOS=WIN95
!include <ntwin32.mak>
CFLAGS =  -DVIC -DWIN32 -DWINDOWS -DUSE_TIME

.c.obj:
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr $<

TOOLS32 =  d:\progra~1\micros~1\vc98
#VC5: TOOLS32 =  d:\progra~1\devstudio\vc
cc32 = cl -I$(TOOLS32)\include

OBJS = tmndec.obj getpic.obj getvlc.obj gethdr.obj getblk.obj getbits.obj store.obj \
	recon.obj idct.obj idctref.obj sac.obj win.obj yuvrgb24.obj
SRCS = tmndec.c getpic.c getvlc.c gethdr.c getblk.c getbits.c store.c \
	recon.c idct.c idctref.c sac.c win.c yuvrgb24.c

libh263.lib : $(OBJS)
	del $@
	link /LIB /OUT:$@ $(OBJS)


