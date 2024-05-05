#include <stdint.h>

#define REG_FFVM_STDIO ((volatile uint32_t*)0xFF000000)

void main(void)
{
    *REG_FFVM_STDIO = 'h';
    *REG_FFVM_STDIO = 'e';
    *REG_FFVM_STDIO = 'l';
    *REG_FFVM_STDIO = 'l';
    *REG_FFVM_STDIO = 'o';
    *REG_FFVM_STDIO = '\n';
}

