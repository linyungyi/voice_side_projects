#
# Name:    makefile
# Creates: isdncta
# Usage:   make -f makefile
#
TARGET=isdncta
OBJS= isdncta.o

EXTRA_LIBS=-lisdnapi -lswiapi -lcta -lnccapi -ladiapi -lvceapi
include ../nmsmacro.mak

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $? -o $@
%.o: %.c
	$(CC) -c  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(IPATHS) $? -o $@
