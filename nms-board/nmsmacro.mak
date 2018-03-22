#*****************************************************************************
#
# Name:  nmsmacro.mak (include makefile for Unix demos)
#
#******************************************************************************

CC           = gcc

CFLAGS       = $(EXTRA_CFLAGS)

DEFLIST      = $(EXTRA_DEFLIST)  -DUNIX_SVR4 -DUNIX -DSOLARIS -DMULTITHREAD \
	-DSOLARIS_SPARC
IPATHS       = $(EXTRA_IPATHS) -I/opt/nms/include -I/opt/nms/ctaccess/demos/ctademo \
	-I/opt/nms/ctaccess/demos/include
CTADEMOOBJ   = ../ctademo/ctademo.o
LPATHS       = $(EXTRA_LPATHS) -L/opt/nms/lib
LIBS         = -lcta -ladiapi -lnccapi -lvceapi -lswiapi -ladidtm $(EXTRA_LIBS) -ldl -lthread
LFLAGS       = $(EXTRA_LFLAGS) -lthread









