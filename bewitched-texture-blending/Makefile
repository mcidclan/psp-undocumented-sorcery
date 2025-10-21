BINOUT = ./bin/
PATHSRC = ./
PATHOBJS = $(BINOUT)
TARGET = $(BINOUT)mod

GP_MODE = 0
GU_POINTS = 1

ifeq ($(MODE),565)
MAIN = main_565
else ifeq ($(MODE),GU_POINTS)
MAIN = main_gu_points
GP_MODE = $(GU_POINTS)
else
MAIN = main
endif

CPP_FILES = $(MAIN).cpp
PATHFILES = $(CPP_FILES) $(S_FILES)

OBJS = $(notdir $(patsubst %.cpp, %.o, $(patsubst %.S, %.o, $(PATHFILES))))
OBJS := $(sort $(OBJS:%.o=$(PATHOBJS)%.o))

CFLAGS = -Ofast -G0 -Wall -fno-pic -DBUILD_PRX=1 \
         -Wextra -Werror -DMODE=$(GP_MODE) -DGU_POINTS_MODE=$(GU_POINTS)

CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti  -std=c++11
ASFLAGS = $(CFLAGS)

LIBS = -lpsppower -lpspgu -lm

PSP_EBOOT_SFO = $(BINOUT)PARAM.SFO
EXTRA_TARGETS = $(BINOUT)EBOOT.PBP
PSP_EBOOT = $(EXTRA_TARGETS)
PSP_EBOOT_TITLE = Bewitched $(MODE) Texture

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

$(PATHOBJS)%.o: $(PATHSRC)%.S
	$(CXX) -o $@ -c $< $(ASFLAGS)

$(PATHOBJS)%.o: $(PATHSRC)%.cpp
	$(CXX) -o $@ -c $< $(CXXFLAGS)

all: $(EXTRA_TARGETS)
	@echo "Compiled in $(MODE) mode, main file: $(MAIN).cpp"
	@echo "Available modes: MODE=GU_POINTS, MODE=565, MODE=<empty> (default)"

