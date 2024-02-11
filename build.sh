#!/bin/sh

set -e

case "$1" in
"")
    make -C $PWD/libffvm
    make -C $PWD/2048
    make -C $PWD/snake
    make -C $PWD/bricks
    make -C $PWD/time
    ;;
clean|distclean)
    make -C $PWD/libffvm clean
    make -C $PWD/2048    clean
    make -C $PWD/snake   clean
    make -C $PWD/bricks  clean
    make -C $PWD/time    clean
    ;;
esac
