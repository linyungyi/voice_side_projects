#
# Name:    makefile
# Creates: incta
# Usage:   make -f makefile
#
DEMOTARGET=incta
EXTRA_IPATHS=-I../csplayrc
EXTRA_LFLAGS=-lstdc++
SOURCE=incta.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
