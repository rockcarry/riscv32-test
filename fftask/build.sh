#!/bin/sh

set -e

CROSS_COMPILE=riscv32-picolibc-elf-
GCC=${CROSS_COMPILE}gcc
GXX=${CROSS_COMPILE}g++
AR=${CROSS_COMPILE}ar
OBJCOPY=${CROSS_COMPILE}objcopy

LIBFFVM_TOP=$PWD/../libffvm
LIBFATFS_TOP=$PWD/../fatfs

C_FLAGS="-Wall -Os -I$LIBFFVM_TOP"
LDFLAGS="$LDFLAGS --specs=picolibc.specs --crt0=hosted -T$LIBFFVM_TOP/ffvm.ld"
LDFLAGS="$LDFLAGS -L$LIBFFVM_TOP/lib -L$LIBFATFS_TOP/lib"
LDFLAGS="$LDFLAGS -Wl,--gc-sections,--start-group"
LDFLAGS="$LDFLAGS --oslib=ffvm -lfatfs"

case "$1" in
"")
    $GCC -c $C_FLAGS fftask.c pthread.c
    $GCC -c fftask.s -o fftask.s.o
    $AR rcs libfftask.a *.o
    mkdir -p $PWD/lib $PWD/bin
    mv libfftask.a $PWD/lib

    $GCC $C_FLAGS $LDFLAGS test0.c $PWD/lib/libfftask.a -o fftask-test0.elf
    $OBJCOPY -O binary fftask-test0.elf fftask-test0.rom

    $GCC $C_FLAGS $LDFLAGS test1.c $PWD/lib/libfftask.a -o fftask-test1.elf
    $OBJCOPY -O binary fftask-test1.elf fftask-test1.rom

    $GCC $C_FLAGS $LDFLAGS test2.c $PWD/lib/libfftask.a -o fftask-test2.elf
    $OBJCOPY -O binary fftask-test2.elf fftask-test2.rom

    $GCC $C_FLAGS $LDFLAGS test3.c $PWD/lib/libfftask.a -o fftask-test3.elf
    $OBJCOPY -O binary fftask-test3.elf fftask-test3.rom

    $GCC $C_FLAGS $LDFLAGS test4.c $PWD/lib/libfftask.a -o fftask-test4.elf
    $OBJCOPY -O binary fftask-test4.elf fftask-test4.rom

    $GCC $C_FLAGS $LDFLAGS test5.c $PWD/lib/libfftask.a -o fftask-test5.elf
    $OBJCOPY -O binary fftask-test5.elf fftask-test5.rom

    mv *.elf *.rom $PWD/bin
    rm *.o
    ;;
clean|distclean)
    rm -rf $PWD/*.o $PWD/lib $PWD/bin
    ;;
esac
