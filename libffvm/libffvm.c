#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define _POSIX_MONOTONIC_CLOCK
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/syslimits.h>
#include <unistd.h>
#include <fcntl.h>
#include <fatfs.h>
#include "ffvmreg.h"

static int stdin_getc(FILE *file)
{
    int  ch;
    do { ch = *REG_FFVM_STDIO; if (ch == EOF) usleep(10 * 1000); } while (ch == EOF);
    return ch;
}

static int stdout_putc(char c, FILE *file)
{
    return (*REG_FFVM_STDIO = c);
}

static int stdout_flush(FILE *file)
{
    *REG_FFVM_STDIO = -1;
    return 0;
}

static int stderr_putc(char c, FILE *file)
{
    return (*REG_FFVM_STDERR = c);
}

static int stderr_flush(FILE *file)
{
    *REG_FFVM_STDERR = -1;
    return 0;
}

static FILE __stdio  = FDEV_SETUP_STREAM(stdout_putc, stdin_getc, stdout_flush, _FDEV_SETUP_RW   );
static FILE __stderr = FDEV_SETUP_STREAM(stderr_putc, NULL      , stderr_flush, _FDEV_SETUP_WRITE);

FILE *const stdin  = &__stdio;
FILE *const stdout = &__stdio;
FILE *const stderr = &__stderr;

int  getch (void) { return *REG_FFVM_GETCH; }
int  kbhit (void) { return *REG_FFVM_KBHIT; }
void clrscr(void) { *REG_FFVM_CLRSCR = 0;   }
void gotoxy(int x, int y) { *REG_FFVM_GOTOXY = (x << 0) | (y << 16); }

void mdelay(int ms)
{
    uint32_t tick_end = *REG_FFVM_MTIMECURL + ms;
    while ((int32_t)tick_end - (int32_t)*REG_FFVM_MTIMECURL > 0);
}

#if ENABLE_FATFS
static FATFS s_fatfs_context;
static FIL  *s_fil_list[256];
int open(const char *file, int flags, ...)
{
    int  i;
    for (i = 3; i < 256 && s_fil_list[i]; i++);
    if  (i == 256) return -1;

    FIL *fil = malloc(sizeof(FIL));
    if (!fil) return -1;

    uint8_t mode = FA_READ;
    if (flags & O_RDONLY) mode = FA_READ;
    if (flags & O_WRONLY) mode = FA_WRITE;
    if (flags & O_RDWR  ) mode = FA_READ | FA_WRITE;
    if (flags & O_CREAT ) {
        if (flags & O_EXCL) mode |= FA_CREATE_NEW;
        else mode |= FA_CREATE_ALWAYS;
    } else {
        if (flags & O_APPEND) mode |= FA_OPEN_APPEND;
    }
    int ret = f_open(fil, file, mode);
    if (ret != FR_OK) { free(fil); return -1; }

    s_fil_list[i] = fil;
    return i;
}

int close(int fd)
{
    if (fd < 3 || fd >= 256) return -1;
    FIL *fil = s_fil_list[fd];
    if (fil) { f_close(fil); free(fil); }
    s_fil_list[fd] = NULL;
    return 0;
}

int fstat(int fd, struct stat *sbuf)
{
    if (fd < 3 || fd >= 256) return -1;
    FIL *fil = s_fil_list[fd];
    if (!fil) return -1;
    sbuf->st_size = f_size(fil);
    return 0;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
    if (fd < 3 || fd >= 256) return -1;
    FIL *fil = s_fil_list[fd];
    if (!fil) return -1;
    unsigned n;
    FRESULT ret = f_read(fil, buf, nbyte, &n);
    return ret == FR_OK ? n : -1;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
    if (fd < 3 || fd >= 256) return -1;
    FIL *fil = s_fil_list[fd];
    if (!fil) return -1;
    unsigned n;
    FRESULT ret = f_write(fil, buf, nbyte, &n);
    return ret == FR_OK ? n : -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    if (fd < 3 || fd >= 256) return -1;
    FIL *fil = s_fil_list[fd];
    if (!fil) return -1;
    switch (whence) {
    case SEEK_SET: f_lseek(fil, offset); break;
    case SEEK_END: f_lseek(fil, offset + f_size(fil)); break;
    case SEEK_CUR: f_lseek(fil, offset + f_tell(fil)); break;
    }
    return f_tell(fil);
}

int unlink(const char *pathname)
{
    return f_unlink(pathname);
}

int rename(const char *oldname, const char *newname)
{
    return f_rename(oldname, newname);
}

int mkdir(const char *path, mode_t mode)
{
    return f_mkdir(path);
}

int chdir(const char *path)
{
    return f_chdir(path);
}

char* getcwd(char *buf, size_t size)
{
    char cwd[256];
    f_getcwd(cwd, sizeof(cwd));
    snprintf(buf, size, "/disk%s", strcmp(cwd, "/") == 0 ? "" : cwd);
    return buf;
}

typedef struct {
    long  d_ino;
    off_t d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char d_name[NAME_MAX + 1];
    uint16_t d_fdate;
    uint16_t d_ftime;
    uint64_t d_fsize;
} DIRENT;

typedef struct {
    DIR    dir;
    DIRENT ent;
} DIRCTX;

void* opendir(const char *path)
{
    DIRCTX *dirctx = calloc(1, sizeof(DIRCTX));
    int  ret = f_opendir(&dirctx->dir, path);
    if (ret == FR_OK) {
        return dirctx;
    } else {
        free(dirctx);
        return NULL;
    }
}

int closedir(void *dir)
{
    DIRCTX *dirctx = dir;
    if (dirctx) {
        f_closedir(&dirctx->dir);
        free(dirctx);
    }
    return 0;
}

DIRENT* readdir(void *dir)
{
    if (!dir) return NULL;
    DIRCTX *dirctx = dir;
    FILINFO info;
    if (f_readdir(&dirctx->dir, &info) != FR_OK || !info.fname[0]) return NULL;
    dirctx->ent.d_ino    = 0;
    dirctx->ent.d_off    = 0;
    dirctx->ent.d_reclen = sizeof(DIRENT);
    dirctx->ent.d_reclen = dirctx->ent.d_reclen < NAME_MAX ? dirctx->ent.d_reclen : NAME_MAX;
    dirctx->ent.d_type   = (info.fattrib & AM_DIR) ? 4 : 8;
    dirctx->ent.d_fdate  = info.fdate;
    dirctx->ent.d_ftime  = info.ftime;
    dirctx->ent.d_fsize  = info.fsize;
    snprintf(dirctx->ent.d_name, sizeof(dirctx->ent.d_name), "%s", info.fname);
    return &dirctx->ent;
}

void rewinddir(void *dir)
{
    DIRCTX *dirctx = dir;
    if (dirctx) f_rewinddir(&dirctx->dir);
}
#else
int open(const char *file, int flags, ...) { return -1; }
int close(int fd) { return -1; }
ssize_t read(int fd, void *buf, size_t nbyte) { return -1; }
ssize_t write(int fd, const void *buf, size_t nbyte) { return -1; }
off_t lseek(int fd, off_t offset, int whence) { return -1; }
DIR *opendir(const char *path) { return NULL; }
int closedir(DIR *dir) { return -1; }
struct dirent* readdir(DIR *dir) { return NULL; }
void rewinddir(DIR *dir) {}
#endif

void _init(void)
{
#if ENABLE_FATFS
    f_mount(&s_fatfs_context, "", 0);
#endif
}

void _ATTRIBUTE ((__noreturn__)) _exit(int code)
{
#if ENABLE_FATFS
    f_unmount("");
#endif

    *REG_FFVM_CPU_FREQ = 0;
    while (1);
}

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    tv->tv_sec  = *REG_FFVM_REALTIME;
    tv->tv_usec = *REG_FFVM_MTIMECURL * 1000;
    return 0;
}

int clock_gettime(clockid_t clk_id, struct timespec *tp)
{
    switch (clk_id) {
    case CLOCK_MONOTONIC:
        tp->tv_sec  = (((uint64_t)*REG_FFVM_MTIMECURH << 32) | ((uint64_t)*REG_FFVM_MTIMECURL << 0)) / 1000;
        tp->tv_nsec = *REG_FFVM_MTIMECURL % 1000 * 1000 * 1000;
        break;
    case CLOCK_REALTIME:
        tp->tv_sec  = *REG_FFVM_REALTIME;
        tp->tv_nsec = *REG_FFVM_MTIMECURL % 1000 * 1000 * 1000;
        break;
    }
    return 0;
}
