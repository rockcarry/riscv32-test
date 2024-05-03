#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ffvmreg.h"
#include "libffvm.h"

int main(void)
{
    while (1) {
        time_t     tt = time(NULL);
        struct tm *tm = localtime(&tt);
        printf("tick: %lu, time: %lu, %04d-%02d-%02d %02d:%02d:%02d\n", *REG_FFVM_MTIMECURL, *REG_FFVM_REALTIME,
            tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        mdelay(1000);
    }
    return 0;
}
