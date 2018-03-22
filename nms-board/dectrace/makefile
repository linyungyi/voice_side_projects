#
# Name:    makefile
# Creates: dectrace
# Usage:   make -f makefile
#
TARGET=dectrace
OBJS= dectrace.o  decisdn.o

EXTRA_LIBS=
include ../nmsmacro.mak

$(TARGET): $(OBJS)
	$(CC)  $(CFLAGS) $(DEFLIST) -DCT_ACCESS  $(LFLAGS) $(LPATHS) $(LIBS) $? -o $@
%.o: %.c
	$(CC) -c  $(CFLAGS) $(DEFLIST) -DCT_ACCESS $(IPATHS) $? -o $@
