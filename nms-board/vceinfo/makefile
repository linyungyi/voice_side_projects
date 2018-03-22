#
# Name:    makefile
# Creates: vceinfo
# Usage:   make -f makefile
#
DEMOTARGET=vceinfo
EXTRA_LFLAGS=-lstdc++
SOURCE=vceinfo.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
