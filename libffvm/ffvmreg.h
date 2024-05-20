#ifndef __FFVMREG_H__
#define __FFVMREG_H__

#include <stdint.h>

#define RISCV_CSR_MSTATUS         0x300
#define RISCV_CSR_MISA            0x301
#define RISCV_CSR_MIE             0x304
#define RISCV_CSR_MTVEC           0x305
#define RISCV_CSR_MSCRATCH        0x340
#define RISCV_CSR_MEPC            0x341
#define RISCV_CSR_MCAUSE          0x342
#define RISCV_CSR_MTVAL           0x343
#define RISCV_CSR_MIP             0x344

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
#define REG_FFVM_DISP_BITBLT_ADDR ((volatile uint32_t*)0xFF000214)
#define REG_FFVM_DISP_BITBLT_XY   ((volatile uint32_t*)0xFF000218)
#define REG_FFVM_DISP_BITBLT_WH   ((volatile uint32_t*)0xFF00021C)

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

#define REG_FFVM_MTIMECURL        ((volatile uint32_t*)0xFF000400)
#define REG_FFVM_MTIMECURH        ((volatile uint32_t*)0xFF000404)
#define REG_FFVM_MTIMECMPL        ((volatile uint32_t*)0xFF000408)
#define REG_FFVM_MTIMECMPH        ((volatile uint32_t*)0xFF00040C)
#define REG_FFVM_REALTIME         ((volatile uint32_t*)0xFF000410)

#define REG_FFVM_DISK_SECTOR_NUM  ((volatile uint32_t*)0xFF000500)
#define REG_FFVM_DISK_SECTOR_SIZE ((volatile uint32_t*)0xFF000504)
#define REG_FFVM_DISK_SECTOR_IDX  ((volatile uint32_t*)0xFF000508)
#define REG_FFVM_DISK_SECTOR_DAT  ((volatile uint32_t*)0xFF00050C)

#define FLAG_FFVM_IRQ_AOUT        (1 << 0)
#define FLAG_FFVM_IRQ_AIN         (1 << 1)
#define FLAG_FFVM_IRQ_KEYBD       (1 << 2)
#define FLAG_FFVM_IRQ_MOUSE       (1 << 3)
#define FLAG_FFVM_IRQ_ETHPHY      (1 << 4)

#define REG_FFVM_CPU_FREQ         ((volatile uint32_t*)0xFF000600)
#define REG_FFVM_IRQ_ENABLE       ((volatile uint32_t*)0xFF000604)
#define REG_FFVM_IRQ_FLAGS        ((volatile uint32_t*)0xFF000608)
#define REG_FFVM_IRQ_AOUT_THRES   ((volatile uint32_t*)0xFF00060C)
#define REG_FFVM_IRQ_AIN_THRES    ((volatile uint32_t*)0xFF000610)
#define REG_FFVM_IRQ_ETHP_THRES   ((volatile uint32_t*)0xFF000614)

#define REG_FFVM_ETHPHY_OUT_ADDR  ((volatile uint32_t*)0xFF000700)
#define REG_FFVM_ETHPHY_OUT_SIZE  ((volatile uint32_t*)0xFF000704)
#define REG_FFVM_ETHPHY_IN_ADDR   ((volatile uint32_t*)0xFF000708)
#define REG_FFVM_ETHPHY_IN_HEAD   ((volatile uint32_t*)0xFF00070C)
#define REG_FFVM_ETHPHY_IN_TAIL   ((volatile uint32_t*)0xFF000710)
#define REG_FFVM_ETHPHY_IN_SIZE   ((volatile uint32_t*)0xFF000714)
#define REG_FFVM_ETHPHY_IN_CURR   ((volatile uint32_t*)0xFF000718)
#define REG_FFVM_ETHPHY_IN_LOCK   ((volatile uint32_t*)0xFF00071C)

#endif
