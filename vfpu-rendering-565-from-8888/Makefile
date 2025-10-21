BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)
TARGET = $(BINOUT)vfpusr

CPP_FILES = $(shell ls $(PATHSRC)*.cpp)
PATHFILES = $(CPP_FILES)

OBJS = $(notdir $(patsubst %.cpp, %.o, $(patsubst %.S, %.o, $(PATHFILES))))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))

CFLAGS = -Ofast -G0 -Wall -fno-pic -I./kernel/src
# \
#         -Wextra -Werror

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti  -std=c++11
ASFLAGS = $(CFLAGS)

LIBS = -lstdc++ -lpsppower -lpspgu

PSP_EBOOT_SFO = $(BINOUT)PARAM.SFO
EXTRA_TARGETS = $(BINOUT)EBOOT.PBP
PSP_EBOOT = $(EXTRA_TARGETS)
PSP_EBOOT_TITLE = VFPU 565 Rendering

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

$(PATHOBJS)%.o: $(PATHSRC)%.S
	$(CXX) -o $@ -c $< $(ASFLAGS)

$(PATHOBJS)%.o: $(PATHSRC)%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)
