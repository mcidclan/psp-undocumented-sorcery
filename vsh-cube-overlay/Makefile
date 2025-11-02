BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)
TARGET = $(BINOUT)vshcube

C_FILES = $(wildcard $(PATHSRC)*.c)
S_FILES = $(wildcard $(PATHSRC)*.S)
PATHFILES = $(C_FILES) $(S_FILES)

OBJS = $(notdir $(patsubst %.c, %.o, $(C_FILES)))
OBJS += $(notdir $(patsubst %.S, %.o, $(S_FILES)))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))
OBJS += $(BINOUT)exports.o

USE_KERNEL_LIBS = 1
CFLAGS = -fsingle-precision-constant -Os -G0 -Wall
CXXFLAGS = $(CFLAGS) -fno-exceptions
ASFLAGS = $(CFLAGS)
LDFLAGS += -nostartfiles
CFLAGS += -I$(PSPSDK)/../include
LIBS = -lpsppower -lpspgum -lpspgu -lpspge -lm -lpspsystemctrl_kernel
 
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

$(PATHOBJS)%.o: $(PATHSRC)%.c
	$(CC) -o $@ -c $< $(CXXFLAGS)

$(PATHOBJS)%.o: $(PATHSRC)%.S
	$(CC) -o $@ -c $< $(ASFLAGS)

$(PATHOBJS)exports.c: $(PATHSRC)exports.exp
	psp-build-exports -b $< > $@
