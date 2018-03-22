#
# Name:    makefile
# Creates: libevtapi.so libevtmgr.so 
# Usage:   make -f makefile
#
EXTRA_IPATHS=-I.
DLLFLAGS=-G -KPIC -Bdynamic
DBGCFLAGS=-g  -DPARANOID -UDEBUG -DSTATIC=static  -DUNIX -DUNIX_SVR4 -DSOLARIS -DUSLSVR4 -DMULTITHREAD -D_REENTRANT -DSOLARIS_SPARC -fdefer-pop -fmerge-constants -fthread-jumps -floop-optimize -fcrossjumping -fif-conversion -fif-conversion2 -fdelayed-branch -fguess-branch-probability -fcprop-registers -DCT_ACCESS  -I. -I../../inc -I/export/home/share/NA2005/nademos/p4/software/api/tsa/evt/evtsvc/../../../../common/inc -DSTATIC=static -I/export/home/share/NA2005/Shared/hdrs -D_REENTRANT -DUSLSVR4 -DPARANOID -DCT_ACCESS

MGR_LIB=libevtmgr.so
MGR_SRCS=evtcmd.c evtbnd.c
MGR_LIBS=-lcta -ltsi -lthread
MGR_DEF=evtmgr.def

API_LIB=libevtapi.so
API_SRCS=evtapi.c evtspi.c
API_LIBS=-lcta -lthread
API_DEF=evtapi.def

default: $(SVC_SRC_FILE) $(MGR_LIB) $(API_LIB)

include ../nmsmacro.mak

$(MGR_LIB): $(MGR_SRCS)
	$(CC)  -v $(DBGCFLAGS) $(DLLFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS)  \
	$^ -o $@
	cp $(MGR_LIB) /opt/nms/lib

$(API_LIB): $(API_SRCS)
	$(CC) -v $(DBGCFLAGS) $(DLLFLAGS) $(DEFLIST) $(IPATHS) $(API_LIBS) \
	$^ -o $@
	cp $(API_LIB) /opt/nms/lib
