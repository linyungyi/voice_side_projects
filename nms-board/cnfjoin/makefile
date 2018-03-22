#
# Name:    makefile
# Creates: cnfjoin
# Usage:   make -f makefile
#
CC=gcc
DEMOTARGET=cnfjoin
EXTRA_LIBS=-lcnfapi
SOURCE=cnfjoin.c
OBJ=cnfjoin.o

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
