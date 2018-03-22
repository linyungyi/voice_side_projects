#
# Name:    makefile
# Creates: prt2prt
# Usage:   make -f makefile
#
DEMOTARGET=prt2prt
EXTRA_LFLAGS=-lstdc++
SOURCE=prt2prt.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
