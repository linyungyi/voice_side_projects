#
# Name:    makefile
# Creates: snmpWalk
# Usage:   make -f makefile
#
DEMOTARGET=snmpWalk
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpWalk.cpp

include ../../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
