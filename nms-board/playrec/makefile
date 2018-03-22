#
# Name:    makefile
# Creates: playrec
# Usage:   make -f makefile
#
DEMOTARGET=playrec
EXTRA_LFLAGS=-lstdc++
SOURCE=playrec.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
