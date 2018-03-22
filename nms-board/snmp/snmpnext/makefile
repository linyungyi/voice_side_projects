#
# Name:    makefile
# Creates: snmpNext
# Usage:   make -f makefile
#
DEMOTARGET=snmpNext
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpNext.cpp

include ../../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
