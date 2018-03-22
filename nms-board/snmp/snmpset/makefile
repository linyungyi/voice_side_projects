#
# Name:    makefile
# Creates: snmpSet
# Usage:   make -f makefile
#
DEMOTARGET=snmpSet
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpSet.cpp

include ../../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
