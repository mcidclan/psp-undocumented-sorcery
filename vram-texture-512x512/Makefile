BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)
TARGET = $(BINOUT)target

BUILD_PRX = 1

CPP_FILES = $(wildcard $(PATHSRC)*.cpp)
PATHFILES = $(CPP_FILES) $(S_FILES)

OBJS = $(notdir $(patsubst %.cpp, %.o, $(patsubst %.S, %.o, $(PATHFILES))))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))

CFLAGS = -Ofast -G0 -Wall -fno-pic \
         -Wextra -Werror

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti  -std=c++11
ASFLAGS = $(CFLAGS)

LIBS = -lpsppower -lpspgum -lpspgu

PSP_EBOOT_SFO = $(BINOUT)PARAM.SFO
EXTRA_TARGETS = $(BINOUT)EBOOT.PBP
PSP_EBOOT = $(EXTRA_TARGETS)
PSP_EBOOT_TITLE = Gu Vram Tex 512

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

$(PATHOBJS)%.o: $(PATHSRC)%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)
