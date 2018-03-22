#
# Name:    makefile
# Creates: isdndemo
# Usage:   make -f makefile
#
TARGET=isdndemo
OBJS= isdndemo.o  Context.o  BChannel.o  DChannel.o  NAI.o  Call.o  Timer.o  Board.o

EXTRA_LIBS=-lisdnapi -loamapi -lcta -lnccapi -ladiapi -lvceapi
include ../nmsmacro.mak

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $? -o $@
%.o: %.cpp
	$(CC) -c  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(IPATHS) $? -o $@
