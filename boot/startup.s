.text
.extern main
.global _start
_start:
    li sp, 0x80000000
    call main
_loop:
    j _loop

