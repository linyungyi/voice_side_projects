#
# Name:    makefile
# Creates: oaminfo
# Usage:   make -f makefile
#
DEMOTARGET=oaminfo
EXTRA_LIBS=-loamapi
SOURCE=oaminfo.c
OBJ=oaminfo.o

include ../nmsmacro.mak

$(DEMOTARGET): $(OBJ)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
$(OBJ): $(SOURCE)
	$(CC) -v -c $(CFLAGS) $(DEFLIST) $(IPATHS) $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
