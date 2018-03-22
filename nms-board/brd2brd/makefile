#
# Name:    makefile
# Creates: brd2brd
# Usage:   make -f makefile
#
DEMOTARGET=brd2brd
EXTRA_LIBS=-lppxapi
EXTRA_LFLAGS=-lstdc++
SOURCE=brd2brd.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
