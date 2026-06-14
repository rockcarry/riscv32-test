include ../config.mk

CROSS_COMPILE ?= riscv32-picolibc-elf-
GCC           ?= $(CROSS_COMPILE)gcc
OBJCOPY       ?= $(CROSS_COMPILE)objcopy

C_FLAGS := -Wall -Os -g $(ARCH_FLAGS) -I$(TOP)/libffvm
LDFLAGS := --specs=picolibc.specs --crt0=hosted -T$(TOP)/libffvm/ffvm.ld
LDFLAGS += -L$(TOP)/libffvm/lib
LDFLAGS += -Wl,--gc-sections --oslib=ffvm

PROGS = fputest.elf fstress.elf fspill.elf
ROMS  = $(PROGS:.elf=.rom)

all: $(ROMS)

%.rom: %.elf
	$(OBJCOPY) -O binary $< $@

fputest.elf: fputest.c
	$(GCC) $(C_FLAGS) -o $@ $< $(LDFLAGS)

fstress.elf: fstress.c
	$(GCC) $(C_FLAGS) -o $@ $< $(LDFLAGS)

fspill.elf: fspill.c
	$(GCC) $(C_FLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f *.elf *.rom
