#
# Name:    makefile
# Creates: tiktst
# Usage:   make -f makefile
#
DEMOTARGET=tiktst
EXTRA_IPATHS=-I../tiksvc
EXTRA_LIBS=-ltikmgr -ltikapi
EXTRA_LFLAGS=-lstdc++
EXTRA_CFLAGS=-g -DPARANOID -KPIC -DUSLSVR4 -DMULTITHREAD -D_REENTRANT -DCSCTA
SOURCE=tiktst.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
