/*
 * libffvm/dirent.h — 自定义 dirent 类型定义（兼容 picolibc 不支持 dirent）
 *
 * libffvm.c 中已实现 opendir/readdir/closedir/rewinddir（通过 FatFS）。
 * 本头文件提供 POSIX 兼容的类型声明，使上层代码（如 shell.c）可正常编译。
 *
 * 对应 libffvm.c 中的 DIRENT 结构体：
 *   d_ino, d_off, d_reclen, d_type, d_name, d_fdate, d_ftime, d_fsize
 */
#ifndef __LIBFFVM_DIRENT_H__
#define __LIBFFVM_DIRENT_H__

#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 目录项类型常量（与 FatFS AM_DIR / 普通文件对应） */
#define DT_DIR  4
#define DT_REG  8

/*
 * struct dirent — 目录项，扩展了 FatFS 的日期/大小字段。
 * 布局与 libffvm.c 中的 DIRENT 完全一致。
 */
struct dirent {
    long           d_ino;      /* 保留，设为 0               */
    off_t          d_off;      /* 保留，设为 0               */
    unsigned short d_reclen;   /* 本记录长度                 */
    unsigned char  d_type;     /* DT_DIR 或 DT_REG           */
    char           d_name[256];/* 文件名（NAME_MAX + 1）     */
    uint16_t       d_fdate;    /* FatFS 文件日期             */
    uint16_t       d_ftime;    /* FatFS 文件时间             */
    uint64_t       d_fsize;    /* FatFS 文件大小             */
};

/*
 * DIR — 目录流（内部为 FatFS DIR + 缓存 dirent 的复合结构）。
 * libffvm.c 中用 void* 实现，此处声明为不透明指针即可。
 */
typedef struct __libffvm_dir DIR;

/* 函数声明（与 libffvm.c 中 void* 版本 ABI 兼容） */
DIR           *opendir  (const char *path);
struct dirent *readdir  (DIR *dir);
int            closedir (DIR *dir);
void           rewinddir(DIR *dir);

#ifdef __cplusplus
}
#endif

#endif /* __LIBFFVM_DIRENT_H__ */
