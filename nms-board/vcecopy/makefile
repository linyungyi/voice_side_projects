#
# Name:    makefile
# Creates: vcecopy
# Usage:   make -f makefile
#
DEMOTARGET=vcecopy
EXTRA_LFLAGS=-lstdc++
SOURCE=vcecopy.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
