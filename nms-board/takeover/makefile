#
# Name:    makefile
# Creates: takeover
# Usage:   make -f makefile
#
DEMOTARGET=takeover
EXTRA_LIBS=-loamapi
EXTRA_LFLAGS=-lstdc++
SOURCE=takeover.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
