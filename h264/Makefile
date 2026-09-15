BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)

TARGET = $(BINOUT)h264

#PSP_FW_VERSION = 660

C_FILES = $(wildcard $(PATHSRC)*.c)
OBJS = $(notdir $(patsubst %.c, %.o, $(C_FILES)))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))

BUILD_PRX = 1

CFLAGS = -O2 -G0 -g0 -Wall -fsingle-precision-constant
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LDFLAGS =
LIBS = -lpspmpeg -lpspmpegbase -lpsppower

PSP_EBOOT_SFO = $(BINOUT)PARAM.SFO
EXTRA_TARGETS = $(BINOUT)EBOOT.PBP
PSP_EBOOT = $(EXTRA_TARGETS)
PSP_EBOOT_TITLE = h264 Simple

PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

$(PATHOBJS)%.o: $(PATHSRC)%.c
	$(CC) -o $@ -c $< $(CFLAGS)
