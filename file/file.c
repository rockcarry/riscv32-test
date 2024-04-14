#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "ffvmreg.h"
#include "libffvm.h"
#include "ff.h"

int main(void)
{
    char  *file = "/disk0/hello.txt";
    static FATFS fatfs;
    char   buf[256];
    FIL    fp;
    int    ret;
    f_mount(&fatfs, "", 0);
    ret = f_open(&fp, file, FA_READ);
    if (ret == 0) {
        f_gets(buf, sizeof(buf), &fp);
        printf("%s\n", buf);
        f_close(&fp);
    } else {
        printf("failed to open file: %s, ret: %d !\n", file, ret);
    }
    ret = f_open(&fp, "/disk0/test.txt", FA_OPEN_ALWAYS|FA_READ|FA_WRITE);
    if (ret == 0) {
        time_t     tt = time(NULL);
        struct tm *tm = localtime(&tt);
        snprintf(buf, sizeof(buf), "%04d-%02d-%02d %02d:%02d:%02d\n", tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
        f_puts(buf, &fp);
        printf("write %s to file /disk0/test.txt\n", buf);
        f_close(&fp);
    } else {
        printf("failed to open file: %s, ret: %d !\n", "/disk0/test.txt", ret);
    }
    f_unmount("");
    return 0;
}
