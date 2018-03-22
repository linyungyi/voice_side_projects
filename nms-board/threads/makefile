#
# Name:    makefile
# Creates: threads
# Usage:   make -f makefile
#
DEMOTARGET=threads
EXTRA_LFLAGS=-lstdc++
SOURCE=threads.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
