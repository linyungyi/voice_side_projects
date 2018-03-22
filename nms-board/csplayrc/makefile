#
# Name:    makefile
# Creates: csplayrc
# Usage:   make -f makefile
#
DEMOTARGET=csplayrc
EXTRA_IPATHS=-I../include
EXTRA_LFLAGS=-lstdc++
SOURCE=csplayrc.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
