#
# Name:    makefile
# Creates: waveinfo
# Usage:   make -f makefile
#
DEMOTARGET=waveinfo
EXTRA_LFLAGS=-lstdc++
SOURCE=waveinfo.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
