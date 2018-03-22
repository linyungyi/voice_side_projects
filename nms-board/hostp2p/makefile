#
# Name:    makefile
# Creates: hostp2p
# Usage:   make -f makefile
#
DEMOTARGET=hostp2p
EXTRA_LFLAGS=-lstdc++
SOURCE=hostp2p.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
