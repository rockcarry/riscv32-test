#!/bin/sh

set -e

case "$1" in
"")
    make -C $PWD/ffvmos
    make -C $PWD/2048
    make -C $PWD/snack
    ;;
clean)
    make -C $PWD/ffvmos clean
    make -C $PWD/2048   clean
    make -C $PWD/snack  clean
    ;;
distclean)
    make -C $PWD/ffvmos clean
    make -C $PWD/2048   clean
    make -C $PWD/snack  clean
    ;;
esac
