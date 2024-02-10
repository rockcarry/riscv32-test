#ifndef __FFVMREG_H__
#define __FFVMREG_H__

#define REG_FFVM_STDIO  ((volatile uint32_t*)0xFF000000)
#define REG_FFVM_STDERR ((volatile uint32_t*)0xFF000004)
#define REG_FFVM_GETCH  ((volatile uint32_t*)0xFF000008)
#define REG_FFVM_KBHIT  ((volatile uint32_t*)0xFF00000C)

#define REG_FFVM_MSLEEP ((volatile uint32_t*)0xFF000100)
#define REG_FFVM_CLRSCR ((volatile uint32_t*)0xFF000104)
#define REG_FFVM_GOTOXY ((volatile uint32_t*)0xFF000108)

#endif
