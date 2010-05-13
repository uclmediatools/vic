# $Id$

CPP	:= g++
SRCS	:= $(wildcard *.cc)
OBJS	:= $(SRCS:.cc=.o)
CFLAGS	:= -Wall -g -O2
RM	:= rm -f

.PHONY:	clean distclean

all: $(SRCS)
	$(CPP) $(CFLAGS) asv.cc -o asv
	$(CPP) $(CFLAGS) xr_send.cc -o xr_send

clean:
	$(RM) asv xr_send

