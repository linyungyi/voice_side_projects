#
# Name:    makefile
# Creates: isdnncc
# Usage:   make -f makefile
#
TARGET=isdnncc
OBJS= isdnncc.o

EXTRA_LIBS=-lswiapi -lcta -lnccapi -ladiapi -lvceapi
include ../nmsmacro.mak

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $? -o $@
%.o: %.c
	$(CC) -c  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(IPATHS) $? -o $@
