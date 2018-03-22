#
# Name:    makefile
# Creates: snmpHsMon
# Usage:   make -f makefile
#
DEMOTARGET=snmpHsMon
SNMPDEMOUTILOBJ=SnmpDemoUtil.o
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc -I../common/
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpHsMon.cpp

include ../../nmsmacro.mak

default: $(SNMPDEMOUTILOBJ) $(DEMOTARGET) 

$(SNMPDEMOUTILOBJ): ../common/SnmpDemoUtil.cpp
	$(CC) -v -c $(CFLAGS) $(DEFLIST) $(IPATHS) $<

$(DEMOTARGET): $(SOURCE) ../common/SnmpDemoUtil.cpp
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $^ -o $@


