# Generated automatically from Makefile.in by configure.
#
# Makefile for the SAP example.
# This probably requires GNU make.
#

# Location of includes and library
CSRC   = ../../src

# Library name
LNAME  = uclmmbase

DEFS   = -DHAVE_CONFIG_H
CFLAGS = -g -O2 -W -Wall -Wwrite-strings -Wbad-function-cast -Wmissing-prototypes -Wcast-qual -Wmissing-declarations -Werror $(DEFS) -I$(CSRC)
LIBS   = -lnsl -lsocket -L$(CSRC) -l$(LNAME)
CC     = gcc

TARGET = sapdemo
OBJS   = sapdemo.o
SRCS   = $(OBJS:%.o=%.c)

all: $(TARGET)

sapdemo: $(OBJS) $(CSRC)/lib$(LNAME).a
	 $(CC) $(CFLAGS) -o $@ $(OBJS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INC) -c $<

$(CSRC)/lib$(LNAME).a:
	cd $(CSRC) && $(MAKE)

clean:
	-rm -f $(OBJS) $(TARGET)

distclean: clean
	-rm -f Makefile
