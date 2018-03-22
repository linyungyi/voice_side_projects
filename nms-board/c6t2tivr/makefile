#
# Name:    makefile
# Creates: c6t2tivr
# Usage:   make -f makefile
#
DEMOTARGET=c6t2tivr
EXTRA_LFLAGS=-lstdc++
SOURCE=c6t2tivr.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
