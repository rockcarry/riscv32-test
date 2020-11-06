CROSS_COMPILE ?= riscv32-unknown-elf-
GCC           ?= $(CROSS_COMPILE)gcc
GXX           ?= $(CROSS_COMPILE)g++
OBJCOPY       ?= $(CROSS_COMPILE)objcopy

LIBFFVM_TOP = $(shell pwd)/../libffvm

C_FLAGS := -Wall -Os --sysroot=$(PICOLIBC_DIR) --specs=$(PICOLIBC_DIR)/picolibc.specs
C_FLAGS += -I$(LIBFFVM_TOP)

LDFLAGS := --sysroot=$(PICOLIBC_DIR) --specs=$(PICOLIBC_DIR)/picolibc.specs
LDFLAGS += -L$(LIBFFVM_TOP)/lib -lffvm -T$(LIBFFVM_TOP)/ffvm.ld

PROGS = bricks.elf
PROMS = bricks.rom

COMP_C_SRCS=$(wildcard ./*.c)
COMP_C_SRCS_NO_DIR=$(notdir $(COMP_C_SRCS))
OBJECTS=$(patsubst %.c, %.c.o, $(COMP_C_SRCS_NO_DIR))

COMP_CPP_SRCS=$(wildcard ./*.cpp)
COMP_CPP_SRCS_NO_DIR=$(notdir $(COMP_CPP_SRCS))
CXXOBJS=$(patsubst %.cpp, %.cpp.o, $(COMP_CPP_SRCS_NO_DIR))

OBJDIR ?= $(shell pwd)/obj
BINDIR ?= $(shell pwd)/bin
INCDIR ?= $(shell pwd)/inc

BINPROG = $(addprefix $(BINDIR)/, $(PROGS))
BINPROM = $(addprefix $(BINDIR)/, $(PROMS))

.PHONY: clean

all: $(BINPROG) $(BINPROM)

clean:
	@rm -Rf $(OBJDIR)
	@rm -Rf $(BINDIR)

$(BINPROM) : $(BINPROG)
	@mkdir -p $(BINDIR)
	@echo "  ROM $@"
	$(OBJCOPY) -O binary $< $@

$(BINPROG) : $(addprefix $(OBJDIR)/, $(OBJECTS)) $(addprefix $(OBJDIR)/, $(CXXOBJS))
	@mkdir -p $(BINDIR)
	@echo "  ELF $@"
	$(GCC) -o $@ $(addprefix $(OBJDIR)/, $(OBJECTS)) $(addprefix $(OBJDIR)/, $(CXXOBJS)) $(LDFLAGS)

$(OBJDIR)/%.c.o : %.c
	@mkdir -p obj
	@echo "  CC  $<"
	$(GCC) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@

$(OBJDIR)/%.cpp.o : %.cpp
	@mkdir -p obj
	@echo "  CC  $<"
	@$(GXX) $(C_FLAGS) $(C_INCLUDES) -c $< -o $@
