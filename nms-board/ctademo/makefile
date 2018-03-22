#
# Name:    makefile
# Creates: ctademo
# Usage:   make -f makefile
#
DEMOTARGET=ctademo.o
EXTRA_LFLAGS=-lstdc++
EXTRA_CFLAGS=-DMAKEOBJ
SOURCE=ctademo.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v -c $(CFLAGS) $(DEFLIST) $(IPATHS) $<
