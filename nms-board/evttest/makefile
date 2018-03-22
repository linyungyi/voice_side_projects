#
# Name:    makefile
# Creates: evttest
# Usage:   make -f makefile
#
DEMOTARGET=evttest
EXTRA_IPATHS=-I../evtsvc -I../tiksvc
EXTRA_LIBS=-levtmgr -levtapi -ltikmgr -ltikapi
EXTRA_LFLAGS=-lstdc++
EXTRA_CFLAGS=-g -DPARANOID -KPIC -DUSLSVR4 -DMULTITHREAD -D_REENTRANT -DCSCTA
SOURCE=evttest.c

include ../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
