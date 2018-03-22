#
# Name:    makefile
# Creates: snmpGet
# Usage:   make -f makefile
#
DEMOTARGET=snmpGet
EXTRA_IPATHS=-I../../include/nmssnmp -I../../include/nmsnfc
EXTRA_LIBS=-lNmsNfc -lNmsSnmp
SOURCE=snmpGet.cpp

include ../../nmsmacro.mak

$(DEMOTARGET): $(SOURCE)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
