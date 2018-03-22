#
# Name:    makefile
# Creates: lapddemo
# Usage:   make -f makefile
#
TARGET=lapddemo
OBJS= lapddemo.o  lapdlib.o

EXTRA_LIBS=-lisdnapi -lcta -lnccapi -ladiapi -lvceapi
include ../nmsmacro.mak

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $? -o $@
%.o: %.c
	$(CC) -c  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(IPATHS) $? -o $@
