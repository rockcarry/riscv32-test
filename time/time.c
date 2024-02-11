#include <stdlib.h>
#include <stdio.h>
#include "ffvmreg.h"
#include "libffvm.h"

int main(void)
{
    while (1) {
        printf("tick: %lu, time: %lu\n", *REG_FFVM_TICKTIME, *REG_FFVM_REALTIME);
        msleep(1000);
    }
    exit(0);
}
