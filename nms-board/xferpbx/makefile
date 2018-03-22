#
# Name:    makefile
# Creates: xferpbx
# Usage:   make -f makefile
#
DEMOTARGET=xferpbx
EXTRA_LFLAGS=-lstdc++
SOURCE=xferpbx.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
