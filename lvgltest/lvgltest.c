#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <lvgl/lvgl.h>
#include "ffvmreg.h"
#include "libffvm.h"

#define SCREEN_WIDTH  480
#define SCREEN_HEIGHT 360

 void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    lv_color_t *disp = disp_drv->user_data;
    lv_color_t *pdst = disp + area->y1 * SCREEN_WIDTH + area->x1;
    int         n    = area->x2 - area->x1 + 1, i;
    for (i = area->y1; i <= area->y2; i++) {
        memcpy(pdst, color_p, n * sizeof(lv_color_t));
        color_p += n, pdst += SCREEN_WIDTH;
    }
    lv_disp_flush_ready(disp_drv);
}

static void mouse_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    data->point.x = (*REG_FFVM_MOUSE_XY >> 0 ) & 0xFFFF;
    data->point.y = (*REG_FFVM_MOUSE_XY >> 16) & 0xFFFF;
    data->state   = (*REG_FFVM_MOUSE_BTN & (1 << 0)) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static uint32_t get_tick_count(void) { return *REG_FFVM_TICKTIME; }

extern void lv_demo_widgets(void);
extern void lv_demo_widgets_close(void);
extern void lv_demo_benchmark(void);
extern void lv_demo_benchmark_close(void);

int main(void)
{
    uint32_t *disp_buf = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t) + 2 * SCREEN_WIDTH * SCREEN_HEIGHT / 2 * sizeof(lv_color_t));
    if (!disp_buf) { printf("failed to allocate display buffer !\n"); return 0; }
    *REG_FFVM_DISP_ADDR        = (uint32_t)disp_buf;
    *REG_FFVM_DISP_WH          = (SCREEN_WIDTH << 0) | (SCREEN_HEIGHT << 16);
    *REG_FFVM_DISP_REFRESH_WH  = (SCREEN_WIDTH << 0) | (SCREEN_HEIGHT << 16);
    *REG_FFVM_DISP_REFRESH_DIV = 4;

    lv_disp_draw_buf_t disp_draw = {};
    lv_disp_drv_t      disp_drv  = {};
    lv_disp_t         *disp_ctx  = NULL;
    lv_indev_drv_t     indev_mouse_drv;
    lv_indev_t        *indev_mouse_dev;

    lv_init();
    lv_color_t *buf1 = (lv_color_t*)(disp_buf + SCREEN_WIDTH * SCREEN_HEIGHT);
    lv_color_t *buf2 = buf1 + SCREEN_WIDTH * SCREEN_HEIGHT / 2;
    lv_disp_draw_buf_init(&disp_draw, buf1, buf2, SCREEN_WIDTH * SCREEN_HEIGHT / 2);
    lv_disp_drv_init(&disp_drv);

    disp_drv.hor_res   = SCREEN_WIDTH;
    disp_drv.ver_res   = SCREEN_HEIGHT;
    disp_drv.flush_cb  = disp_flush;
    disp_drv.draw_buf  =&disp_draw;
    disp_drv.user_data = disp_buf;

    disp_ctx = lv_disp_drv_register(&disp_drv);
    lv_disp_set_rotation(disp_ctx, 0);

    lv_indev_drv_init(&indev_mouse_drv);
    indev_mouse_drv.type    = LV_INDEV_TYPE_POINTER;
    indev_mouse_drv.read_cb = mouse_read;
    indev_mouse_dev = lv_indev_drv_register(&indev_mouse_drv);
    (void)indev_mouse_dev;

    lv_demo_widgets();

    uint32_t tick_last = get_tick_count(), tick_cur;
    int32_t  tick_inc;
    while (*REG_FFVM_DISP_WH) {
        tick_cur  = get_tick_count();
        tick_inc  = (int32_t)tick_cur - (int32_t)tick_last;
        if (tick_inc > 0) {
            lv_tick_inc(tick_inc);
            tick_last = tick_cur;
            lv_timer_handler();
        }
    }

    lv_demo_widgets_close();
    free(disp_buf);
    return 0;
}
