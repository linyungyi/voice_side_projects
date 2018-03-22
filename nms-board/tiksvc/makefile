#
# Name:    makefile
# Creates: libtikapi.so libtikmgr.so 
# Usage:   make -f makefile
#
EXTRA_IPATHS=-I. -I../tiksvr
DLLFLAGS=-G -KPIC -Bdynamic
DBGCFLAGS=-g  -DPARANOID -UDEBUG -DSTATIC=static  -DUNIX -DUNIX_SVR4 -DSOLARIS -DUSLSVR4 -DMULTITHREAD -D_REENTRANT -DSOLARIS_SPARC -fdefer-pop -fmerge-constants -fthread-jumps -floop-optimize -fcrossjumping -fif-conversion -fif-conversion2 -fdelayed-branch -fguess-branch-probability -fcprop-registers -DCT_ACCESS  -I. -I../tiksvr -I../../inc -I/export/home/share/NA2005/nademos/p4/software/api/tsa/tik/tiksvc/../../../../common/inc -DSTATIC=static -I/export/home/share/NA2005/Shared/hdrs -D_REENTRANT -DUSLSVR4 -DPARANOID -DCT_ACCESS

MGR_LIB=libtikmgr.so
MGR_SRCS=tikcmd.c tikcomms.c tikutils.c tikbnd.c tikparm.c
MGR_LIBS=-lcta -ltsi -lthread
MGR_DEF=tikmgr.def

API_LIB=libtikapi.so
API_SRCS=tikapi.c tikspi.c
API_LIBS=-lcta -lthread
API_DEF=tikapi.def

SVC_SRC_FILE=tikparm.c tikparm.h
SVC_PF_FILE=tikparm.pf
PF_UTIL=pf2src

default: $(SVC_SRC_FILE) $(MGR_LIB) $(API_LIB)

include ../nmsmacro.mak

$(SVC_SRC_FILE) : $(SVC_PF_FILE)
	$(PF_UTIL) $(SVC_PF_FILE)

$(MGR_LIB): $(MGR_SRCS)
	$(CC) -v $(DBGCFLAGS) $(DLLFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS)  \
	$^ -o $@
	cp $(MGR_LIB) /opt/nms/lib

$(API_LIB): $(API_SRCS)
	$(CC) -v $(DBGCFLAGS) $(DLLFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(API_LIBS) \
	$^ -o $@
	cp $(API_LIB) /opt/nms/lib
