#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

static int sample_putc(char c, FILE *file)
{
    (void) file;
    return c;
}

static int sample_getc(FILE *file)
{
    unsigned char c = -1;
    (void) file;
    return c;
}

static FILE __stdin  = FDEV_SETUP_STREAM(NULL, sample_getc, NULL, _FDEV_SETUP_READ );
static FILE __stdout = FDEV_SETUP_STREAM(sample_putc, NULL, NULL, _FDEV_SETUP_WRITE);
static FILE __stderr = FDEV_SETUP_STREAM(sample_putc, NULL, NULL, _FDEV_SETUP_WRITE);
FILE *const __iob[3] = { &__stdin, &__stdout, &__stderr };

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
    while (1);
}

