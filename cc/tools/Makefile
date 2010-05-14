# $Id$

CPP	:= g++
SRCS	:= $(wildcard *.cc)
OBJS	:= $(SRCS:.cc=.o)
CFLAGS	:= -Wall -g -O2
RM	:= rm -f

.PHONY: all clean distclean

ALL = asv xr_send
all: $(ALL)

asv: asv.cc
	$(CPP) $(CFLAS) $< -o $@

xr_send: xr_send.cc
	$(CPP) $(CFLAS) $< -o $@

clean:
	$(RM) $(ALL)

