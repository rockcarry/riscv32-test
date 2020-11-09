#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "ffvmreg.h"

static int sample_putc (char c, FILE *file);
static int sample_getc (FILE *file);
static int sample_flush(FILE *file);

static FILE __stdio  = FDEV_SETUP_STREAM(sample_putc, sample_getc, sample_flush, _FDEV_SETUP_RW   );
static FILE __stderr = FDEV_SETUP_STREAM(sample_putc, sample_getc, sample_flush, _FDEV_SETUP_WRITE);
FILE *const __iob[3] = { &__stdio, &__stdio, &__stderr };

static int sample_putc(char c, FILE *file)
{
    if (file == &__stdio) {
        *REG_FFVM_STDIO = c;
    } else if (file == &__stderr) {
        *REG_FFVM_STDERR= c;
    }
    return c;
}

static int sample_getc(FILE *file)
{
    int c = -1;
    if (file == &__stdio) {
        c = *REG_FFVM_STDIO;
    }
    return c;
}

static int sample_flush(FILE *file)
{
    if (file == &__stdio) {
        *REG_FFVM_STDIO = -1;
    } else if (file == &__stderr) {
        *REG_FFVM_STDERR= -1;
    }
    return 0;
}

int  getch (void)  { return *REG_FFVM_GETCH; }
int  kbhit (void)  { return *REG_FFVM_KBHIT; }
void msleep(int ms){ *REG_FFVM_MSLEEP = ms ; }
void clrscr(void)  { *REG_FFVM_CLRSCR = 0  ; }
void gotoxy(int x, int y) { *REG_FFVM_GOTOXY = (x << 0) | (y << 16); }

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

_READ_WRITE_RETURN_TYPE read(int fd, void *buf, size_t nbyte)
{
    (void) fd;
    (void) buf;
    (void) nbyte;
    return -1;
}

_READ_WRITE_RETURN_TYPE write(int fd, const void *buf, size_t nbyte)
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

