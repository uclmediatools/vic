# Generated automatically from Makefile.in by configure.
#
# Makefile for the SDP example.
# This probably requires GNU make.
#

# Location of includes and library
CSRC   = ../../src

# Library name
LNAME  = uclmmbase

DEFS   = -DHAVE_CONFIG_H
CFLAGS = -g -W -Wall -Wwrite-strings -Wbad-function-cast -Wmissing-prototypes -Wcast-qual -Wmissing-declarations -Werror $(DEFS) -I$(CSRC)
LIBS   =  -L$(CSRC) -l$(LNAME)
CC     = gcc

TARGET = sdpdemo
OBJS   = sdpdemo.o
SRCS   = $(OBJS:%.o=%.c)

all: $(TARGET)

sdpdemo: $(OBJS) $(CSRC)/lib$(LNAME).a
	 $(CC) $(CFLAGS) -o $@ $(@).o $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $<

$(CSRC)/lib$(LNAME).a:
	cd $(CSRC) && $(MAKE)

clean:
	-rm -f $(OBJS) $(TARGET)

distclean: clean
	-rm -f Makefile
