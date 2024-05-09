#!/bin/sh

set -e

case "$1" in
"")
    make -C $PWD/libffvm
    make -C $PWD/fatfs
    make -C $PWD/2048
    make -C $PWD/snake
    make -C $PWD/bricks
    make -C $PWD/time
    make -C $PWD/input
    make -C $PWD/disp
    make -C $PWD/audio
    make -C $PWD/lvgltest
    make -C $PWD/file
    cd $PWD/pthread && ./build.sh && cd -
    mkdir -p $PWD/out
    rm   -rf $PWD/out/*
    find . -path "./out" -prune -o -iname "*.rom" -exec cp '{}' out ';'
    ;;
clean|distclean)
    make -C $PWD/libffvm  clean
    make -C $PWD/fatfs    clean
    make -C $PWD/2048     clean
    make -C $PWD/snake    clean
    make -C $PWD/bricks   clean
    make -C $PWD/time     clean
    make -C $PWD/input    clean
    make -C $PWD/disp     clean
    make -C $PWD/audio    clean
    make -C $PWD/lvgltest clean
    make -C $PWD/file     clean
    cd $PWD/pthread && ./build.sh $1 && cd -
    rm -rf $PWD/out
    ;;
esac
