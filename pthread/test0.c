#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ffvmreg.h"
#include "fftask.h"

int main(void)
{
    uint32_t ticks[10];
    int      i, d, pass = 1;
    task_kernel_init();
    for (i = 0; i < 10; i++) {
//      task_kernel_dump("main", "all", 1);
        ticks[i] = *REG_FFVM_MTIMECURL;
        printf("%d cur_tick: %lu\n", i, ticks[i]);
        task_sleep(1000);
    }
    for (i = 1; i < 10; i++) {
        d = abs(ticks[i] - ticks[i - 1] - 1000);
        if (d > 20) {
            printf("test failed, d: %d !\n", d);
            pass = 0; break;
        }
    }
    task_kernel_exit();
    if (pass) printf("test pass.\n");
    return 0;
}

