BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)
TARGET = $(BINOUT)pixbuf

BUILD_PRX = 1

CPP_FILES = $(wildcard $(PATHSRC)*.cpp)
PATHFILES = $(CPP_FILES)

OBJS = $(notdir $(patsubst %.cpp, %.o, $(patsubst %.S, %.o, $(PATHFILES))))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))

CFLAGS = -Os -G0 -Wall -fno-pic \
         -Wextra -Werror

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -std=c++11
ASFLAGS = $(CFLAGS)

LIBS = -lpsppower

PSP_EBOOT_SFO = $(BINOUT)PARAM.SFO
EXTRA_TARGETS = $(BINOUT)EBOOT.PBP
PSP_EBOOT = $(EXTRA_TARGETS)
PSP_EBOOT_TITLE = Pixel Buffer Test
PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

$(PATHOBJS)%.o: $(PATHSRC)%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

clean:
	-rm -f $(TARGET).elf $(TARGET).prx $(OBJS) $(EXTRA_TARGETS) $(PSP_EBOOT_SFO)
