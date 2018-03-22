#
# Name:    makefile
# Creates: trunkmon
# Usage:   make -f makefile
#
DEMOTARGET=trunkmon
EXTRA_LFLAGS=-lstdc++
EXTRA_CFLAGS=-DCT_ACCESS  -DSTRIPPER
SOURCE=trunkmon.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
