/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include <stdint.h>
#include <time.h>
#include "ff.h"         /* Obtains integer types */
#include "diskio.h"     /* Declarations of disk functions */

#define REG_FFVM_DISK_SECTOR_NUM  ((volatile uint32_t*)0xFF000500)
#define REG_FFVM_DISK_SECTOR_SIZE ((volatile uint32_t*)0xFF000504)
#define REG_FFVM_DISK_SECTOR_IDX  ((volatile uint32_t*)0xFF000508)
#define REG_FFVM_DISK_SECTOR_DAT  ((volatile uint32_t*)0xFF00050C)


/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
    BYTE pdrv       /* Physical drive nmuber to identify the drive */
)
{
    volatile uint32_t *disk = REG_FFVM_DISK_SECTOR_NUM + pdrv * 4;
    return *disk > 0 ? 0 : STA_NODISK;
}


/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
    BYTE pdrv               /* Physical drive nmuber to identify the drive */
)
{
    return 0;
}


/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
    BYTE  pdrv,     /* Physical drive nmuber to identify the drive */
    BYTE *buff,     /* Data buffer to store read data */
    LBA_t sector,   /* Start sector in LBA */
    UINT  count     /* Number of sectors to read */
)
{
    volatile uint32_t *disk = REG_FFVM_DISK_SECTOR_NUM + pdrv * 4;
    int  i, j;
    for (i = 0; i < count; i++) {
        disk[2] = sector + i;
        for (j = 0; j < FF_MIN_SS; j++) *buff++ = disk[3];
    }
    return RES_OK;
}


/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write (
    BYTE pdrv,          /* Physical drive nmuber to identify the drive */
    const BYTE *buff,   /* Data to be written */
    LBA_t sector,       /* Start sector in LBA */
    UINT count          /* Number of sectors to write */
)
{
    volatile uint32_t *disk = REG_FFVM_DISK_SECTOR_NUM + pdrv * 4;
    int  sectsize = disk[1], i, j;
    for (i = 0; i < count; i++) {
        disk[2] = sector + i;
        for (j = 0; j < sectsize; j++) disk[3] = *buff++;
    }
    return RES_OK;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl (
    BYTE pdrv,      /* Physical drive nmuber (0..) */
    BYTE cmd,       /* Control code */
    void *buff      /* Buffer to send/receive control data */
)
{
    volatile uint32_t *disk = REG_FFVM_DISK_SECTOR_NUM + pdrv * 4;
    switch (cmd) {
    case GET_SECTOR_COUNT: *(LBA_t   *)buff = disk[0]; return 0;
    case GET_SECTOR_SIZE : *(uint16_t*)buff = disk[1]; return 0;
    case GET_BLOCK_SIZE  : *(uint32_t*)buff = 1;       return 0;
    }
    return RES_PARERR;
}

DWORD get_fattime (void)
{
    time_t     tt = time(NULL);
    struct tm *tm = localtime(&tt);
    return (DWORD)(((tm->tm_year - 80) << 25) | ((tm->tm_mon + 1) << 21) | (tm->tm_mday << 16) | (tm->tm_hour << 11) | (tm->tm_min << 5) | (tm->tm_sec >> 1));
}
