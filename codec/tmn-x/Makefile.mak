# Makefile for TMN simulation 

# This is free software. See the file COPYING for copying conditions.

NODEBUG=1
TARGETOS=WIN95
!include <ntwin32.mak>
CFLAGS =  -DVIC -DWIN32 -DWINDOWS 

.c.obj:
	$(cc32) $(cdebug) $(cflags) $(cvars) $(CFLAGS) -o $@ -FR$*.sbr $<

SRCS =	main.c io.c Makefile dct.c coder.c quant.c mot_est.c pred.c snr.c \
        countbit.c putbits.c ratectrl.c sac.c README COPYING \
        TODO CHANGES putvlc.c intra_pred.c filter.c
HDRS = 	sim.h macros.h config.h \
        sactbls.h indices.h putvlc.h vlc.h main.h
OBJS =	main.obj io.obj dct.obj coder.obj quant.obj mot_est.obj pred.obj  snr.obj \
        countbit.obj putbits.obj ratectrl.obj sac.obj putvlc.obj intra_pred.obj \
        filter.obj malloc_debug.obj

TOOLS32       =  c:\progra~1\micros~2\vc98
#cc32 = cl -I$(TOOLS32)\include
cc32 = cl

libh263coder.lib: $(OBJS)
	del $@
	link /LIB /OUT:$@ $(OBJS)
