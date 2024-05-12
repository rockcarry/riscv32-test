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
    $AR rcs libpthread.a *.o
    mkdir -p $PWD/lib $PWD/bin
    mv libpthread.a $PWD/lib

    $GCC $C_FLAGS $LDFLAGS test0.c $PWD/lib/libpthread.a -o pthread-test0.elf
    $OBJCOPY -O binary pthread-test0.elf pthread-test0.rom

    $GCC $C_FLAGS $LDFLAGS test1.c $PWD/lib/libpthread.a -o pthread-test1.elf
    $OBJCOPY -O binary pthread-test1.elf pthread-test1.rom

    $GCC $C_FLAGS $LDFLAGS test2.c $PWD/lib/libpthread.a -o pthread-test2.elf
    $OBJCOPY -O binary pthread-test2.elf pthread-test2.rom

    $GCC $C_FLAGS $LDFLAGS test3.c $PWD/lib/libpthread.a -o pthread-test3.elf
    $OBJCOPY -O binary pthread-test3.elf pthread-test3.rom

    $GCC $C_FLAGS $LDFLAGS test4.c $PWD/lib/libpthread.a -o pthread-test4.elf
    $OBJCOPY -O binary pthread-test4.elf pthread-test4.rom

    $GCC $C_FLAGS $LDFLAGS test5.c $PWD/lib/libpthread.a -o pthread-test5.elf
    $OBJCOPY -O binary pthread-test5.elf pthread-test5.rom

    mv *.elf *.rom $PWD/bin
    rm *.o
    ;;
clean|distclean)
    rm -rf $PWD/*.o $PWD/lib $PWD/bin
    ;;
esac
