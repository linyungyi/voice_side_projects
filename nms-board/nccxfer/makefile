#
# Name:    makefile
# Creates: nccxfer
# Usage:   make -f makefile
#
DEMOTARGET=nccxfer
EXTRA_LFLAGS=-lstdc++
SOURCE=nccxfer.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
