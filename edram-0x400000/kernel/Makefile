BINOUT = ./bin/
PATHSRC = ./src/
TARGET_NAME = wrap
TARGET = $(BINOUT)$(TARGET_NAME)
CC = psp-gcc

PSPSDK = $(shell psp-config --pspsdk-path)

CFLAGS = -fsingle-precision-constant -Ofast -G0 -Wall -I$(PSPSDK)/include
ASFLAGS = -x assembler-with-cpp $(CFLAGS)
LDFLAGS = -Wl,-q,-T$(PSPSDK)/lib/linkfile.prx -nostartfiles -Wl,-zmax-page-size=128 -L$(PSPSDK)/lib

release: all
	psp-build-exports -s $(PATHSRC)exports.exp && \
	mv $(TARGET_NAME).S ../ && \
	mv bin/$(TARGET_NAME).prx ../bin

C_FILES = $(wildcard $(PATHSRC)*.c)
S_FILES = $(wildcard $(PATHSRC)*.S)
PATHFILES = $(C_FILES) $(S_FILES)
OBJS = $(notdir $(patsubst %.c, %.o, $(patsubst %.S, %.o, $(PATHFILES))))
OBJS := $(sort $(OBJS:%.o=$(BINOUT)%.o))
OBJS += $(BINOUT)exports.o

LIBS =

FINAL_TARGET = $(TARGET).prx

all: $(FINAL_TARGET)

$(TARGET).elf: $(OBJS)
	$(CC) $(LDFLAGS) $^ -o $@ $(LIBS)

%.prx: %.elf
	psp-prxgen $< $@

$(BINOUT)%.o: $(PATHSRC)%.c
	$(CC) -o $@ -c $< $(CFLAGS)

$(BINOUT)%.o: $(PATHSRC)%.S
	$(CC) $(ASFLAGS) -c $< -o $@

%.c: %.exp
	psp-build-exports -b $< > $@

clean:
	-rm -f $(FINAL_TARGET) $(TARGET).elf $(OBJS)

rebuild: clean all
