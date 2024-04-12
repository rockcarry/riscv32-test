#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ffvmreg.h"
#include "libffvm.h"

int main(void)
{
    int       disp_w = 640;
    int       disp_h = 480;
    uint32_t *disp_buf = malloc(disp_w * disp_h * sizeof(uint32_t));
    int       i, j;
    if (!disp_buf) { printf("failed to allocate display buffer !\n"); return 0; }
    printf("disp_buf: %p\n", disp_buf);
    *REG_FFVM_DISP_ADDR       = (uint32_t)disp_buf;
    *REG_FFVM_DISP_WH         = (disp_w << 0) | (disp_h << 16);
    *REG_FFVM_DISP_REFRESH_WH = (disp_w << 0) | (disp_h << 16);
    *REG_FFVM_DISP_REFRESH_DIV= 4;
    for (i = 0; i < disp_h; i++) {
        for (j = 0; j < disp_w; j++) {
            disp_buf[i * disp_w + j] = (i << 16) | (j << 8) | (i << 0);
        }
    }
    while (*REG_FFVM_DISP_WH);
    free(disp_buf);
    return 0;
}
