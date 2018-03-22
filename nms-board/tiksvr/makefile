# Name:    makefile
# Creates: tiksvr
# Usage:   make -f makefile
#
DEMOTARGET=tiksvr
SOURCE=tiksvr.c
OBJ=tiksvr.o

EXTRA_LIBS = -ltsi -lthread

include ../nmsmacro.mak

$(DEMOTARGET): $(OBJ)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(EXTRA_LIBS) -ltsi $? -o $@

$(OBJ): $(SOURCE)
	$(CC) -v -c $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(EXTRA_LIBS) $?
