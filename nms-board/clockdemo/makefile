#
# Name:    makefile
# Creates: clockdemo
# Usage:   make -f makefile
#
CC=gcc
DEMOTARGET=clockdemo
EXTRA_LFLAGS=-lstdc++
SOURCE1=clockdemo.c
SOURCE2=clockresource.c
OBJ1=clockdemo.o
OBJ2=clockresource.o

include ../nmsmacro.mak
LIBS += -loamapi -lswiapi -lcta -ldtmapi
CFLAGS += 
DEFLIST +=  
IPATHS =  -I/opt/nms/include  -I/opt/nms/ctaccess/demos/include

$(DEMOTARGET): $(SOURCE1) $(SOURCE2)
	$(CC) -v $(CFLAGS) $(DEFLIST) $(IPATHS) $(LFLAGS) $(LPATHS) $(LIBS) $(LTHREADS) $? -o $@
