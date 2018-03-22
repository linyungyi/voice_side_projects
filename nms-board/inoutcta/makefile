#
# Name:    makefile
# Creates: inoutcta
# Usage:   make -f makefile
#
DEMOTARGET=inoutcta
EXTRA_LFLAGS=-lstdc++
SOURCE=inoutcta.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
