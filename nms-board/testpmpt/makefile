#
# Name:    makefile
# Creates: testpmpt
# Usage:   make -f makefile
#
DEMOTARGET=testpmpt
EXTRA_LFLAGS=-lstdc++
SOURCE=testpmpt.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
