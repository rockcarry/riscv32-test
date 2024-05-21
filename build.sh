#!/bin/sh

set -e

case "$1" in
"")
    make -C $PWD/libffvm
    make -C $PWD/fatfs
    cd $PWD/fftask && ./build.sh && cd -

    make -C $PWD/2048
    make -C $PWD/snake
    make -C $PWD/bricks
    make -C $PWD/time
    make -C $PWD/input
    make -C $PWD/disp
    make -C $PWD/audio1
    make -C $PWD/audio2
    make -C $PWD/lvgltest
    make -C $PWD/file
    make -C $PWD/ethphy
    make -C $PWD/lwiptest
    rm   -rf $PWD/out
    mkdir -p $PWD/out
    find . -path "./out" -prune -o -iname "*.rom" -exec cp '{}' out ';'
    ;;

clean|distclean)
    make -C $PWD/libffvm  clean
    make -C $PWD/fatfs    clean
    cd $PWD/fftask && ./build.sh $1 && cd -

    make -C $PWD/2048     clean
    make -C $PWD/snake    clean
    make -C $PWD/bricks   clean
    make -C $PWD/time     clean
    make -C $PWD/input    clean
    make -C $PWD/disp     clean
    make -C $PWD/audio1   clean
    make -C $PWD/audio2   clean
    make -C $PWD/lvgltest clean
    make -C $PWD/file     clean
    make -C $PWD/ethphy   clean
    make -C $PWD/lwiptest clean
    rm -rf $PWD/out
    ;;
esac
