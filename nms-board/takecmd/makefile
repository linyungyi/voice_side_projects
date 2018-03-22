#
# Name:    makefile
# Creates: takecmd
# Usage:   make -f makefile
#
DEMOTARGET=takecmd
EXTRA_IPATHS=-I../takeover
EXTRA_LFLAGS=-lstdc++
SOURCE=takecmd.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
