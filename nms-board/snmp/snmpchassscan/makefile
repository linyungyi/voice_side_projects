#
# Name:    makefile
# Creates: snmpChassScan
# Usage:   make -f makefile
#
DEMOTARGET=snmpChassScan
SNMPDEMOUTILOBJ=SnmpDemoUtil.o
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc -I../common/
EXTRA_LIBS=-lNmsNfc -lNmsSnmp -ltsi
SOURCE=snmpChassScan.cpp

include ../../nmsmacro.mak

default: $(SNMPDEMOUTILOBJ) $(DEMOTARGET) 

$(SNMPDEMOUTILOBJ): ../common/SnmpDemoUtil.cpp
	$(CC) -v -c $(CFLAGS) $(DEFLIST) $(IPATHS) $<

$(DEMOTARGET): $(SOURCE) ../common/SnmpDemoUtil.cpp
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $^ -o $@


