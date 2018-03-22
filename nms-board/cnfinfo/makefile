#
# Name:    makefile
# Creates: cnfinfo
# Usage:   make -f makefile
#
CC=gcc
DEMOTARGET=cnfinfo
EXTRA_LIBS=-lcnfapi
SOURCE=cnfinfo.c
OBJ=cnfinfo.o

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
