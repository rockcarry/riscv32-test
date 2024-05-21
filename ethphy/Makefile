CROSS_COMPILE ?= riscv32-picolibc-elf-
GCC           ?= $(CROSS_COMPILE)gcc
GXX           ?= $(CROSS_COMPILE)g++
OBJCOPY       ?= $(CROSS_COMPILE)objcopy

LIBFFVM_TOP  = $(shell pwd)/../libffvm
LIBFATFS_TOP = $(shell pwd)/../fatfs
LIBFFTASK_TOP= $(shell pwd)/../fftask

C_FLAGS := -Wall -Os -g -I$(LIBFFVM_TOP) -I$(LIBFFTASK_TOP)
LDFLAGS := --specs=picolibc.specs --crt0=hosted -T$(LIBFFVM_TOP)/ffvm.ld
LDFLAGS += -L$(LIBFFVM_TOP)/lib -L$(LIBFATFS_TOP)/lib -L$(LIBFFTASK_TOP)/lib
LDFLAGS += -Wl,--gc-sections,--start-group
LDFLAGS += --oslib=ffvm -lfatfs -lfftask

LIBRARY = libethphy.a
PROGS   = ethphy.elf
PROMS   = ethphy.rom

COMP_C_SRCS=$(wildcard ./*.c)
COMP_C_SRCS_NO_DIR=$(notdir $(COMP_C_SRCS))
OBJECTS=$(patsubst %.c, %.c.o, $(COMP_C_SRCS_NO_DIR))

COMP_CPP_SRCS=$(wildcard ./*.cpp)
COMP_CPP_SRCS_NO_DIR=$(notdir $(COMP_CPP_SRCS))
CXXOBJS=$(patsubst %.cpp, %.cpp.o, $(COMP_CPP_SRCS_NO_DIR))

OBJDIR ?= $(shell pwd)/obj
LIBDIR ?= $(shell pwd)/lib
BINDIR ?= $(shell pwd)/bin

OBJPROG = $(addprefix $(OBJDIR)/, $(LIBRARY))
BINPROG = $(addprefix $(BINDIR)/, $(PROGS))
BINPROM = $(addprefix $(BINDIR)/, $(PROMS))

.PHONY: clean

all: $(OBJPROG) $(BINPROG) $(BINPROM)

clean:
	@rm -Rf $(OBJDIR)
	@rm -Rf $(LIBDIR)
	@rm -Rf $(BINDIR)

$(OBJPROG) : $(addprefix $(OBJDIR)/, $(OBJECTS)) $(addprefix $(OBJDIR)/, $(CXXOBJS))
	@mkdir -p $(LIBDIR)
	@echo "  LIBDIR $@"
	@$(AR) rcs $@ $(OBJDIR)/*.o
	@echo ""
	mv -f ${OBJDIR}/$(LIBRARY) ${LIBDIR}

$(OBJDIR)/%.c.o : %.c
	@mkdir -p $(OBJDIR)
	@echo "  CC  $<"
	$(GCC) $(C_FLAGS) -c $< -o $@

$(BINPROM) : $(BINPROG)
	@mkdir -p $(BINDIR)
	@echo "  ROM $@"
	$(OBJCOPY) -O binary $< $@

$(BINPROG) : $(addprefix $(BINDIR)/, $(OBJECTS)) $(addprefix $(BINDIR)/, $(CXXOBJS))
	@mkdir -p $(BINDIR)
	@echo "  ELF $@"
	$(GCC) -o $@ $(addprefix $(BINDIR)/, $(OBJECTS)) $(addprefix $(BINDIR)/, $(CXXOBJS)) $(LDFLAGS)

$(BINDIR)/%.c.o : %.c
	@mkdir -p $(BINDIR)
	@echo "  CC  $<"
	$(GCC) $(C_FLAGS) -D_TEST_ -c $< -o $@
