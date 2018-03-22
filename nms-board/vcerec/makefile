#
# Name:    makefile
# Creates: vcerec
# Usage:   make -f makefile
#
DEMOTARGET=vcerec
EXTRA_LFLAGS=-lstdc++
SOURCE=vcerec.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
