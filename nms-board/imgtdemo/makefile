#
# Name:    makefile
# Creates: imgtdemo
# Usage:   make -f makefile
#
TARGET=imgtdemo
OBJS= imgtdemo.o

EXTRA_LIBS=-limgtapi -lcta -lnccapi -ladiapi -lvceapi
include ../nmsmacro.mak

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(CTADEMOOBJ) $(LFLAGS) $(LPATHS) $(LIBS) $? -o $@
%.o: %.c
	$(CC) -c  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(IPATHS) $? -o $@
