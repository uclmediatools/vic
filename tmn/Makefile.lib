# Makefile for TMN simulation 

# This is free software. See the file COPYING for copying conditions.

SRCS =	main.c io.c Makefile dct.c coder.c quant.c mot_est.c pred.c snr.c \
        countbit.c putbits.c ratectrl.c sac.c README COPYING \
        TODO CHANGES putvlc.c intra_pred.c filter.c
HDRS = 	sim.h macros.h config.h \
        sactbls.h indices.h putvlc.h vlc.h main.h
OBJS =	main.o io.o dct.o coder.o quant.o mot_est.o pred.o  snr.o \
        countbit.o putbits.o ratectrl.o sac.o putvlc.o intra_pred.o \
        filter.o malloc_debug.o

CFLAGS =  $(CCOPT) -DVIC

libh263coder.a:	$(OBJS)
	rm -f $@
	ar rc $@ $(OBJS)
	ranlib $@
