#!/bin/sh

riscv32-picolibc-elf-gcc -nostdlib -nostartfiles -Ttext=0x80000000 -o boot.elf startup.s main.c
riscv32-picolibc-elf-objcopy -O binary boot.elf boot.rom
riscv32-picolibc-elf-objdump -S boot.elf > boot.s

