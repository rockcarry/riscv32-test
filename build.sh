#!/bin/sh

set -e

case "$1" in
"")
    make -C $PWD/libffvm
    make -C $PWD/2048
    make -C $PWD/snack
    ;;
clean)
    make -C $PWD/libffvm clean
    make -C $PWD/2048    clean
    make -C $PWD/snack   clean
    ;;
distclean)
    make -C $PWD/libffvm clean
    make -C $PWD/2048    clean
    make -C $PWD/snack   clean
    ;;
esac
