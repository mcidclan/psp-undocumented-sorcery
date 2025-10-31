BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)
TARGET = $(BINOUT)vshcube

PATHFILES = $(shell ls $(PATHSRC)*.c)
OBJS = $(notdir $(patsubst %.c, %.o, $(PATHFILES)))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))
OBJS += $(BINOUT)exports.o

USE_KERNEL_LIBS = 1

CFLAGS = -fsingle-precision-constant -Os -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions
ASFLAGS = $(CFLAGS)
LDFLAGS += -nostartfiles
CFLAGS += -I$(PSPSDK)/../include

LIBS = -lpsppower -lpspgum -lpspgu -lpspge -lm
 
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

$(PATHOBJS)%.o: $(PATHSRC)%.c
	$(CC) -o $@ -c $< $(CXXFLAGS)

$(PATHOBJS)exports.c: $(PATHSRC)exports.exp
	psp-build-exports -b $< > $@
