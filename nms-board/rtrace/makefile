#
# Name:    makefile
# Creates: rtrace
# Usage:   make -f makefile
#
DEMOTARGET=rtrace
EXTRA_LFLAGS=-lstdc++
SOURCE=rtrace.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
