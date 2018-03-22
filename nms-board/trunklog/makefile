#
# Name:    makefile
# Creates: trunklog
# Usage:   make -f makefile
#
DEMOTARGET=trunklog
EXTRA_LFLAGS=-lstdc++
SOURCE=trunklog.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
