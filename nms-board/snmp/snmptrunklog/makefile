#
# Name:    makefile
# Creates: snmpTrunkLog
# Usage:   make -f makefile
#
DEMOTARGET=snmpTrunkLog
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc -I../common/
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpTrunkLog.cpp

include ../../nmsmacro.mak

default: $(DEMOTARGET) 

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@


