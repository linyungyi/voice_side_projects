#
# Name:    makefile
# Creates: outcta
# Usage:   make -f makefile
#
DEMOTARGET=outcta
EXTRA_LFLAGS=-lstdc++
SOURCE=outcta.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
