#
# Name:    makefile
# Creates: callcntr
# Usage:   make -f makefile
#
DEMOTARGET=callcntr
EXTRA_LIBS=-lppxapi
EXTRA_LFLAGS=-lstdc++
SOURCE=callcntr.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
