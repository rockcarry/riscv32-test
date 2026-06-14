#!/bin/sh
set -e

FFVM_FLOAT=${FFVM_FLOAT:-1}
FFVM_FATFS=${FFVM_FATFS:-1}

if [ "$FFVM_FLOAT" = "1" ]; then
    ARCH_FLAGS="-march=rv32imacf -mabi=ilp32f"
else
    ARCH_FLAGS="-march=rv32imac -mabi=ilp32"
fi

if [ "$FFVM_FATFS" != "1" ]; then
    export DISABLE_FATFS=true
fi

LIBLWIP_INSTALLDIR=$PWD/liblwip

FFVM_FATFS=0 make -C libffvm clean
FFVM_FATFS=0 make -C libffvm

if [ ! -d "lwip" ]; then
    git clone http://m2msw:8002/lwip
fi

cd lwip/src
git checkout master

./clean.sh
./autogen.sh

export CFLAGS="$ARCH_FLAGS"
export LDFLAGS="--specs=picolibc.specs"
./configure --prefix=$LIBLWIP_INSTALLDIR --host=riscv32-picolibc-elf --disable-shared --with-platform=ffvm

make -j8 install
