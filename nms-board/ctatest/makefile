#
# Name:    makefile
# Creates: ctatest
# Usage:   make -f makefile
#
DEMOTARGET=ctatest
EXTRA_LIBS=-loamapi
EXTRA_LFLAGS=-lstdc++
SOURCE=ctatest.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
