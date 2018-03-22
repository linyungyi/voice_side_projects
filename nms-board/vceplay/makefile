#
# Name:    makefile
# Creates: vceplay
# Usage:   make -f makefile
#
DEMOTARGET=vceplay
EXTRA_LFLAGS=-lstdc++
SOURCE=vceplay.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
