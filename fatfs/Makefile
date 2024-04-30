CROSS_COMPILE ?= riscv32-picolibc-elf-
GCC           ?= $(CROSS_COMPILE)gcc
GXX           ?= $(CROSS_COMPILE)g++
OBJCOPY       ?= $(CROSS_COMPILE)objcopy

C_FLAGS := -Wall -Os

PROGS = libfatfs.a

COMP_C_SRCS=$(wildcard ./*.c)
COMP_C_SRCS_NO_DIR=$(notdir $(COMP_C_SRCS))
OBJECTS=$(patsubst %.c, %.c.o, $(COMP_C_SRCS_NO_DIR))

COMP_CPP_SRCS=$(wildcard ./*.cpp)
COMP_CPP_SRCS_NO_DIR=$(notdir $(COMP_CPP_SRCS))
CXXOBJS=$(patsubst %.cpp, %.cpp.o, $(COMP_CPP_SRCS_NO_DIR))

OBJDIR ?= $(shell pwd)/obj
LIBDIR ?= $(shell pwd)/lib
INCDIR ?= $(shell pwd)/inc

OBJPROG = $(addprefix $(OBJDIR)/, $(PROGS))

.PHONY: clean

all: $(OBJPROG)

clean:
	@rm -Rf $(OBJDIR)
	@rm -Rf $(LIBDIR)

$(OBJPROG) : $(addprefix $(OBJDIR)/, $(OBJECTS)) $(addprefix $(OBJDIR)/, $(CXXOBJS))
	@mkdir -p $(LIBDIR)
	@echo "  LIBDIR $@"
	@$(AR) rcs $@ $(OBJDIR)/*.o
	@echo ""
	mv -f ${OBJDIR}/$(PROGS) ${LIBDIR}

$(OBJDIR)/%.c.o : %.c
	@mkdir -p obj
	@echo "  CC  $<"
	$(GCC) $(C_FLAGS) -c $< -o $@

