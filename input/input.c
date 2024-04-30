#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ffvmreg.h"
#include "libffvm.h"

#define SCREEN_WIDTH  640
#define SCREEN_HEIGHT 480

static void setpixel(uint32_t *disp, int x, int y, int c)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    *(disp + y * SCREEN_WIDTH + x) = c;
}

static void bar(uint32_t *disp, int x, int y, int s, int c)
{
    int  i, j;
    for (i = 0; i < s; i++) {
        for (j = 0; j < s; j++) {
            setpixel(disp, x + j, y + i, c);
        }
    }
}

int main(void)
{
    uint32_t *disp_buf  = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    *REG_FFVM_DISP_ADDR = (uint32_t)disp_buf;
    *REG_FFVM_DISP_WH   = (SCREEN_WIDTH << 0) | (SCREEN_HEIGHT << 16);
    int lastmx = -1, lastmy = -1, curmx, curmy, mbtn, color;
    int lastox = SCREEN_WIDTH / 2, lastoy = SCREEN_HEIGHT / 2, curox = lastox, curoy = lastoy;

    while (*REG_FFVM_DISP_WH) {
        if (*REG_FFVM_KEYBD3 & (1 << 6)) curox += 2; // f
        if (*REG_FFVM_KEYBD3 & (1 << 5)) curoy -= 2; // e
        if (*REG_FFVM_KEYBD3 & (1 << 4)) curoy += 2; // d
        if (*REG_FFVM_KEYBD3 & (1 <<19)) curox -= 2; // s
        bar(disp_buf, lastox, lastoy, 20, 0);
        bar(disp_buf, curox , curoy , 20, 0x00FF00);
        lastox = curox, lastoy = curoy;

        curmx = (*REG_FFVM_MOUSE_XY >> 0 ) & 0xFFFF;
        curmy = (*REG_FFVM_MOUSE_XY >> 16) & 0xFFFF;
        mbtn =  *REG_FFVM_MOUSE_BTN;
        if      (mbtn & (1 << 0)) color = 0x00FF00; // left
        else if (mbtn & (1 << 1)) color = 0xFFFF00; // middle
        else if (mbtn & (1 << 2)) color = 0xFF0000; // right
        else color = 0xCCCCCC;
        bar(disp_buf, lastmx, lastmy, 10, 0);
        bar(disp_buf, curmx, curmy, 10, color);
        lastmx = curmx, lastmy = curmy;

        *REG_FFVM_DISP_REFRESH_WH = (SCREEN_WIDTH << 0) | (SCREEN_HEIGHT << 16);
        mdelay(30);
    }

    *REG_FFVM_DISP_WH = 0;
    return 0;
}
