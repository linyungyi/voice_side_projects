#
# Name:    makefile
# Creates: boardinf
# Usage:   make -f makefile
#
DEMOTARGET=boardinf
EXTRA_LFLAGS=-lstdc++
SOURCE=boardinf.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
