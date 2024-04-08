#ifndef __FFVMREG_H__
#define __FFVMREG_H__

#include <stdint.h>

#define REG_FFVM_STDIO            ((volatile uint32_t*)0xFF000000)
#define REG_FFVM_STDERR           ((volatile uint32_t*)0xFF000004)
#define REG_FFVM_GETCH            ((volatile uint32_t*)0xFF000008)
#define REG_FFVM_KBHIT            ((volatile uint32_t*)0xFF00000C)
#define REG_FFVM_CLRSCR           ((volatile uint32_t*)0xFF000010)
#define REG_FFVM_GOTOXY           ((volatile uint32_t*)0xFF000014)

#define REG_FFVM_KEYBD1           ((volatile uint32_t*)0xFF000110)
#define REG_FFVM_KEYBD2           ((volatile uint32_t*)0xFF000114)
#define REG_FFVM_KEYBD3           ((volatile uint32_t*)0xFF000118)
#define REG_FFVM_KEYBD4           ((volatile uint32_t*)0xFF00011C)
#define REG_FFVM_MOUSE_XY         ((volatile uint32_t*)0xFF000120)
#define REG_FFVM_MOUSE_BTN        ((volatile uint32_t*)0xFF000124)

#define REG_FFVM_DISP_WH          ((volatile uint32_t*)0xFF000200)
#define REG_FFVM_DISP_ADDR        ((volatile uint32_t*)0xFF000204)
#define REG_FFVM_DISP_REFRESH_XY  ((volatile uint32_t*)0xFF000208)
#define REG_FFVM_DISP_REFRESH_WH  ((volatile uint32_t*)0xFF00020C)
#define REG_FFVM_DISP_REFRESH_DIV ((volatile uint32_t*)0xFF000210)

#define REG_FFVM_AUDIO_OUT_FMT    ((volatile uint32_t*)0xFF000300)
#define REG_FFVM_AUDIO_OUT_ADDR   ((volatile uint32_t*)0xFF000304)
#define REG_FFVM_AUDIO_OUT_HEAD   ((volatile uint32_t*)0xFF000308)
#define REG_FFVM_AUDIO_OUT_TAIL   ((volatile uint32_t*)0xFF00030C)
#define REG_FFVM_AUDIO_OUT_SIZE   ((volatile uint32_t*)0xFF000310)
#define REG_FFVM_AUDIO_OUT_CURR   ((volatile uint32_t*)0xFF000314)
#define REG_FFVM_AUDIO_OUT_LOCK   ((volatile uint32_t*)0xFF000318)

#define REG_FFVM_AUDIO_IN_FMT     ((volatile uint32_t*)0xFF000320)
#define REG_FFVM_AUDIO_IN_ADDR    ((volatile uint32_t*)0xFF000324)
#define REG_FFVM_AUDIO_IN_HEAD    ((volatile uint32_t*)0xFF000328)
#define REG_FFVM_AUDIO_IN_TAIL    ((volatile uint32_t*)0xFF00032C)
#define REG_FFVM_AUDIO_IN_SIZE    ((volatile uint32_t*)0xFF000330)
#define REG_FFVM_AUDIO_IN_CURR    ((volatile uint32_t*)0xFF000334)
#define REG_FFVM_AUDIO_IN_LOCK    ((volatile uint32_t*)0xFF000338)

#define REG_FFVM_TICKTIME         ((volatile uint32_t*)0xFF000400)
#define REG_FFVM_REALTIME         ((volatile uint32_t*)0xFF000404)

#endif
