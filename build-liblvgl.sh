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

LIBLVGL_LDFLAGS="-L$PWD/libffvm/lib --specs=picolibc.specs --oslib=ffvm --crt0=hosted -T$PWD/libffvm/ffvm.ld"
LIBLVGL_INSTALLDIR=$PWD/liblvgl

FFVM_FATFS=0 make -C libffvm clean
FFVM_FATFS=0 make -C libffvm

if [ ! -d "littlevgl" ]; then
    git clone http://m2msw:8002/littlevgl
fi

cd littlevgl
git checkout v8.3.x

cp configs/ffvm_defconfig src/lv_conf.h
rm -rf build
cmake -G"Unix Makefiles" -B build \
      -DCMAKE_C_COMPILER=riscv32-picolibc-elf-gcc \
      -DCMAKE_C_FLAGS="$ARCH_FLAGS -Os" \
      -DCMAKE_INSTALL_PREFIX=$LIBLVGL_INSTALLDIR \
      -DCMAKE_EXE_LINKER_FLAGS="$LIBLVGL_LDFLAGS"

make -C build -j8
make -C build install

# CMakeLists.txt doesn't install lvgl_demos, copy manually
cp build/liblvgl_demos.a   $LIBLVGL_INSTALLDIR/lib/
cp build/liblvgl_examples.a $LIBLVGL_INSTALLDIR/lib/
