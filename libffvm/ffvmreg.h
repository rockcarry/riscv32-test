#ifndef __FFVMREG_H__
#define __FFVMREG_H__

#include <stdint.h>

#define REG_FFVM_STDIO            ((volatile uint32_t*)0xFF000000)
#define REG_FFVM_STDERR           ((volatile uint32_t*)0xFF000004)
#define REG_FFVM_GETCH            ((volatile uint32_t*)0xFF000008)
#define REG_FFVM_KBHIT            ((volatile uint32_t*)0xFF00000C)
#define REG_FFVM_CLRSCR           ((volatile uint32_t*)0xFF000010)
#define REG_FFVM_GOTOXY           ((volatile uint32_t*)0xFF000014)

#define REG_FFVM_DISP_WH          ((volatile uint32_t*)0xFF000200)
#define REG_FFVM_DISP_ADDR        ((volatile uint32_t*)0xFF000204)
#define REG_FFVM_DISP_REFRESH_XY  ((volatile uint32_t*)0xFF000208)
#define REG_FFVM_DISP_REFRESH_WH  ((volatile uint32_t*)0xFF00020C)
#define REG_FFVM_DISP_REFRESH_DIV ((volatile uint32_t*)0xFF000210)

#define REG_FFVM_TICKTIME         ((volatile uint32_t*)0xFF000400)
#define REG_FFVM_REALTIME         ((volatile uint32_t*)0xFF000404)

#endif
