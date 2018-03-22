#
# Name:    makefile
# Creates: snmpRecvtrap
# Usage:   make -f makefile
#
DEMOTARGET=snmpRecvtrap
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpRecvtrap.cpp

include ../../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
