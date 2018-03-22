#
# Name:    makefile
# Creates: testdm
# Usage:   make -f makefile
#
DEMOTARGET=testdm
EXTRA_LIBS=-loamapi
EXTRA_LFLAGS=-lstdc++
SOURCE=testdm.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
