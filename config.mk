# riscv32-test 全局配置
# 使用: export FFVM_FLOAT=1/0 FFVM_FFTASK=1/0 FFVM_FATFS=1/0
# 默认全部开启

FFVM_FLOAT  ?= 1
FFVM_FFTASK ?= 1
FFVM_FATFS  ?= 1

TOP := $(realpath $(dir $(lastword $(MAKEFILE_LIST))))

# ---- 架构 ----
ifeq ($(FFVM_FLOAT),1)
  ARCH_FLAGS = -march=rv32imacf -mabi=ilp32f
else
  ARCH_FLAGS = -march=rv32imac -mabi=ilp32
endif

# ---- fftask ----
ifeq ($(FFVM_FFTASK),1)
  FFTASK_DEF  = -DWITH_FFTASK
  FFTASK_LIB  = -lfftask
  FFTASK_INC  = -I$(TOP)/fftask
  FFTASK_LDIR = -L$(TOP)/fftask/lib
else
  FFTASK_DEF  =
  FFTASK_LIB  =
  FFTASK_INC  =
  FFTASK_LDIR =
endif

# ---- fatfs ----
ifeq ($(FFVM_FATFS),1)
  FATFS_DEF  = -DENABLE_FATFS
  FATFS_LIBS = -Wl,--whole-archive -lfatfs -Wl,--no-whole-archive
  FATFS_INC  = -I$(TOP)/fatfs
  FATFS_LDIR = -L$(TOP)/fatfs/lib
else
  FATFS_DEF  =
  FATFS_LIBS =
  FATFS_INC  =
  FATFS_LDIR =
endif
