#!/bin/sh
set -e

FFVM_FLOAT=${FFVM_FLOAT:-1}
FFVM_FATFS=${FFVM_FATFS:-1}

if [ "$FFVM_FLOAT" = "1" ]; then
    ARCH_FLAGS="-march=rv32imacf -mabi=ilp32f"
else
    ARCH_FLAGS="-march=rv32imac -mabi=ilp32"
fi

CROSS_COMPILE=riscv32-picolibc-elf-
GCC=${CROSS_COMPILE}gcc
GXX=${CROSS_COMPILE}g++
AR=${CROSS_COMPILE}ar
OBJCOPY=${CROSS_COMPILE}objcopy

TOP=$PWD/..
LIBFFVM_TOP=$TOP/libffvm
LIBFATFS_TOP=$TOP/fatfs

C_FLAGS="-Wall -Os $ARCH_FLAGS -I$LIBFFVM_TOP"
LDFLAGS="$LDFLAGS --specs=picolibc.specs --crt0=hosted -T$LIBFFVM_TOP/ffvm.ld"
LDFLAGS="$LDFLAGS -L$LIBFFVM_TOP/lib"
if [ "$FFVM_FATFS" = "1" ]; then
    LDFLAGS="$LDFLAGS -L$LIBFATFS_TOP/lib"
    FATFS_LIBS="-Wl,--start-group -Wl,--whole-archive -lfatfs -Wl,--no-whole-archive -Wl,--end-group"
else
    FATFS_LIBS=""
fi
LDFLAGS="$LDFLAGS -Wl,--gc-sections --oslib=ffvm $FATFS_LIBS"

case "$1" in
"")
    $GCC -c $C_FLAGS fftask.c pthread.c
    $GCC -c $C_FLAGS fftask.s -o fftask.s.o
    $AR rcs libfftask.a *.o
    mkdir -p $PWD/lib $PWD/bin
    mv libfftask.a $PWD/lib

    for t in test0 test1 test2 test3 test4 test5; do
        $GCC $C_FLAGS $LDFLAGS ${t}.c $PWD/lib/libfftask.a -o ${t}.elf
        $OBJCOPY -O binary ${t}.elf ${t}.rom
    done

    mv *.elf *.rom $PWD/bin
    rm *.o
    ;;
clean|distclean)
    rm -rf $PWD/*.o $PWD/lib $PWD/bin
    ;;
esac
