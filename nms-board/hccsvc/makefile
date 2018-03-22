#
# Name:    makefile
# Creates: libhccmgr.so 
# Usage:   make -f makefile
#
EXTRA_IPATHS=-I. -I../../include
DLLFLAGS=-G
DBGCFLAGS=-g

MGR_LIB=libhccmgr.so
MGR_OBJ1=hccinit.o
MGR_OBJ2=hccLineObject.o hostTcp.o wnkHostTcp.o hccbnd.o
MGR_LIBS=
MGR_DEF=hccmgr.def

default: $(MGR_LIB)

include ../nmsmacro.mak


$(MGR_LIB): $(MGR_OBJ1) $(MGR_OBJ2)
	$(CC) $(DBGCFLAGS) $(DLLFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS)  \
	$? -o $@
	cp $(MGR_LIB) /opt/nms/lib

hccinit.o: hccinit.c
	$(CC) $(DBGCFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS) \
	$? -c -o $@

hccLineObject.o: hccLineObject.cpp
	$(CC) $(DBGCFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS) \
	$? -c -o $@

hostTcp.o: hostTcp.cpp
	$(CC) $(DBGCFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS) \
	$? -c -o $@

wnkHostTcp.o: wnkHostTcp.cpp
	$(CC) $(DBGCFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS) \
	$? -c -o $@

hccbnd.o: hccbnd.cpp
	$(CC) $(DBGCFLAGS) $(CFLAGS) $(DEFLIST) $(IPATHS) $(MGR_LIBS) \
	$? -c -o $@

clean: 
	rm *.o *.so
