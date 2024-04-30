#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "ffvmreg.h"

static int stdin_getc(FILE *file)
{
    int  ch;
    do { ch = *REG_FFVM_STDIO; } while (ch == EOF);
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
    uint32_t tick_end = *REG_FFVM_TICKTIME + ms;
    while ((int32_t)tick_end - (int32_t)*REG_FFVM_TICKTIME > 0);
}

int open(const char *file, int flags, ...)
{
    (void) file;
    (void) flags;
    return -1;
}

int close(int fd)
{
    (void) fd;
    return -1;
}

int fstat(int fd, struct stat *sbuf)
{
    (void) fd;
    (void) sbuf;
    return -1;
}

ssize_t read(int fd, void *buf, size_t nbyte)
{
    (void) fd;
    (void) buf;
    (void) nbyte;
    return -1;
}

ssize_t write(int fd, const void *buf, size_t nbyte)
{
    (void) fd;
    (void) buf;
    (void) nbyte;
    return -1;
}

off_t lseek(int fd, off_t offset, int whence)
{
    (void) fd;
    (void) offset;
    (void) whence;
    return -1;
}

int unlink(const char *pathname)
{
    (void) pathname;
    return -1;
}

void _ATTRIBUTE ((__noreturn__)) _exit(int code)
{
    (void) code;
    __asm__("li x17, 93");
    __asm__("ecall");
    while (1);
}

int gettimeofday(struct timeval *restrict tv, void *restrict tz)
{
    tv->tv_sec  = *REG_FFVM_REALTIME;
    tv->tv_usec = *REG_FFVM_TICKTIME * 1000;
    return 0;
}
