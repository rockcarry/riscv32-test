include ../config.mk

CROSS_COMPILE ?= riscv32-picolibc-elf-
GCC           ?= $(CROSS_COMPILE)gcc
OBJCOPY       ?= $(CROSS_COMPILE)objcopy

C_FLAGS := -Wall -Os -g $(ARCH_FLAGS) -I$(TOP)/libffvm \
           -mcmodel=medany
LDFLAGS := --specs=picolibc.specs --crt0=hosted -T$(TOP)/libffvm/ffvm.ld \
           -L$(TOP)/libffvm/lib \
           -Wl,--gc-sections --oslib=ffvm

all: smode_test.rom mmu_test.rom

smode_test.elf: smode_test.c
	$(GCC) $(C_FLAGS) -o $@ $< $(LDFLAGS)

mmu_test.elf: mmu_test.c
	$(GCC) $(C_FLAGS) -o $@ $< $(LDFLAGS)

%.rom: %.elf
	$(OBJCOPY) -O binary $< $@

clean:
	rm -f *.elf *.rom
