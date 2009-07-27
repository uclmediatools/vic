# Makefile
# $Id$

# some definitions
CC			:=gcc
CPP			:=g++

RM			:= rm -f
CFLAGS		:=-Wall -g
CPPFLAGS	:=-Wall -g

SRCS 		:= $(wildcard *.cpp)
OBJS		:= $(SRCS:.cpp=.o)
APP			:= test

.PHONY: 	clean distclean

# compile source code to generate object files
.cpp.o:
	$(CPP) $(CPPFLAGS) -c $< -o $@

# link object files together to form an executable
$(APP): $(OBJS)
	$(CPP) $(OBJS) -o $@

# delete object files and temporary files
clean:
	@echo "Start cleaning..."
	$(RM) $(OBJS)
	$(RM) $(APP)
	$(RM) *.*~

# delete object files, temporary files, and executable file
distclean: clean
	$(RM) $(APP)
