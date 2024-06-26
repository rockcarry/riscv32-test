#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ffvmreg.h"
#include "libffvm.h"

#define AUDIO_ADEV_BUFSIZE 8192
#define AUDIO_SAMPLERATE   8000
#define AUDIO_CLIP_SIZE    64
#define AUDIO_START_FREQ   100
#define AUDIO_STOP_FREQ    4000
#define AUDIO_FFT_LEN      1024
#define AUDIO_FREQ_TO_IDX(freq) ((freq) * AUDIO_FFT_LEN / AUDIO_SAMPLERATE)
#define AUDIO_IDX_TO_FREQ(idx ) ( AUDIO_SAMPLERATE * (idx) / AUDIO_FFT_LEN)
#define SCREEN_WIDTH       512
#define SCREEN_HEIGHT      200

float gen_sin_pcm_with_phase(int16_t *pcm, int n, int samprate, int freq, int amp, float phase)
{
    int i; for (i = 0; i < n; i++) pcm[i] = amp * sin(phase + i * 2 * M_PI * freq / samprate);
    return phase + i * 2 * M_PI * freq / samprate;
}

#define ringbuf_size(head, tail, maxsize) (((tail) + (maxsize) - (head) - 0) % maxsize)
#define ringbuf_free(head, tail, maxsize) (((head) + (maxsize) - (tail) - 1) % maxsize)

static int ringbuf_write(uint8_t *rbuf, int maxsize, int tail, uint8_t *src, int len)
{
    uint8_t *buf1 = rbuf    + tail;
    int      len1 = maxsize - tail < len ? maxsize - tail : len;
    uint8_t *buf2 = rbuf;
    int      len2 = len - len1;
    memcpy(buf1, src + 0   , len1);
    memcpy(buf2, src + len1, len2);
    return len2 ? len2 : tail + len1;
}

static int ringbuf_read(uint8_t *rbuf, int maxsize, int head, uint8_t *dst, int len)
{
    uint8_t *buf1 = rbuf    + head;
    int      len1 = maxsize - head < len ? maxsize - head : len;
    uint8_t *buf2 = rbuf;
    int      len2 = len - len1;
    if (dst) {
        memcpy(dst + 0   , buf1, len1);
        memcpy(dst + len1, buf2, len2);
    }
    return len2 ? len2 : head + len1;
}

static void setpixel(uint32_t *disp, int x, int y, int c)
{
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    *(disp + y * SCREEN_WIDTH + x) = c;
}

static void line(uint32_t *disp, int x1, int y1, int x2, int y2, int c)
{
    int dx, dy, d, e;
    if (!disp) return;

    dx = abs(x1 - x2);
    dy = abs(y1 - y2);
    if ((dx >= dy && x1 > x2) || (dx < dy && y1 > y2)) {
        d = x1; x1 = x2; x2 = d;
        d = y1; y1 = y2; y2 = d;
    }
    if (dx >= dy) {
        d = y2 - y1 > 0 ? 1 : -1;
        for (e = dx/2; x1 < x2; x1++, e += dy) {
            if (e >= dx) e -= dx, y1 += d;
            setpixel(disp, x1, y1, c);
        }
    } else {
        d = x2 - x1 > 0 ? 1 : -1;
        for (e = dy/2; y1 < y2; y1++, e += dx) {
            if (e >= dy) e -= dy, x1 += d;
            setpixel(disp, x1, y1, c);
        }
    }
    setpixel(disp, x2, y2, c);
}

int main(void)
{
    int      start_idx = AUDIO_FREQ_TO_IDX(AUDIO_START_FREQ);
    int      stop_idx  = AUDIO_FREQ_TO_IDX(AUDIO_STOP_FREQ );
    int      test_len  = AUDIO_CLIP_SIZE * (stop_idx - start_idx + 1) * sizeof(int16_t);
    int16_t *test_buf  = malloc(test_len);
    int16_t *ptr       = test_buf;
    float    phase     = 0;
    int      lasty, cury, x, i;
    int16_t  pcm[512];

    for (i = start_idx; i <= stop_idx; i++) {
        phase = gen_sin_pcm_with_phase(ptr, AUDIO_CLIP_SIZE, AUDIO_SAMPLERATE, AUDIO_IDX_TO_FREQ(i), 32767 / 2, phase);
        ptr  += AUDIO_CLIP_SIZE;
    }

    uint8_t *adev_buf = malloc(AUDIO_ADEV_BUFSIZE);
    uint8_t *src_buf  = (uint8_t*)test_buf;
    int      src_len  = test_len;

    *REG_FFVM_AUDIO_OUT_ADDR = (uint32_t)adev_buf;
    *REG_FFVM_AUDIO_OUT_SIZE =  AUDIO_ADEV_BUFSIZE;
    *REG_FFVM_AUDIO_OUT_FMT  = (AUDIO_SAMPLERATE << 0) | (1 << 24);
    while (src_len > 0) {
        i = ringbuf_free(*REG_FFVM_AUDIO_OUT_HEAD, *REG_FFVM_AUDIO_OUT_TAIL, *REG_FFVM_AUDIO_OUT_SIZE);
        i = i < src_len ? i : src_len;
        if (i > 0) {
            *REG_FFVM_AUDIO_OUT_TAIL = ringbuf_write(adev_buf, *REG_FFVM_AUDIO_OUT_SIZE, *REG_FFVM_AUDIO_OUT_TAIL, src_buf, i);
            src_len -= i, src_buf += i;
        }
    }
    *REG_FFVM_AUDIO_OUT_SIZE = 0;
    *REG_FFVM_AUDIO_OUT_FMT  = 0;

    uint32_t *disp_buf = malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
    *REG_FFVM_DISP_ADDR     = (uint32_t)disp_buf;
    *REG_FFVM_DISP_WH       = (SCREEN_WIDTH << 0) | (SCREEN_HEIGHT << 16);

    *REG_FFVM_AUDIO_IN_ADDR = (uint32_t)adev_buf;
    *REG_FFVM_AUDIO_IN_SIZE =  AUDIO_ADEV_BUFSIZE;
    *REG_FFVM_AUDIO_IN_FMT  = (AUDIO_SAMPLERATE << 0) | (1 << 24);

    while (*REG_FFVM_DISP_WH) {
        if (ringbuf_size(*REG_FFVM_AUDIO_IN_HEAD, *REG_FFVM_AUDIO_IN_TAIL, *REG_FFVM_AUDIO_IN_SIZE) >= sizeof(pcm)) {
            *REG_FFVM_AUDIO_IN_HEAD = ringbuf_read(adev_buf, *REG_FFVM_AUDIO_IN_SIZE, *REG_FFVM_AUDIO_IN_HEAD, (uint8_t*)pcm, sizeof(pcm));

            memset(disp_buf, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint32_t));
            lasty = SCREEN_HEIGHT / 2 - pcm[0] * SCREEN_HEIGHT / 0x10000;
            for (x = 1; x < SCREEN_WIDTH; x++) {
                i    = (sizeof(pcm) / sizeof(int16_t) - 1) * x / (SCREEN_WIDTH - 1);
                cury = SCREEN_HEIGHT / 2 - pcm[i] * SCREEN_HEIGHT / 0x10000;
                line(disp_buf, x - 1, lasty, x, cury, 0x00FF00);
                lasty = cury;
            }
            *REG_FFVM_DISP_REFRESH_WH = (SCREEN_WIDTH << 0) | (SCREEN_HEIGHT << 16);
        }
    }

    *REG_FFVM_AUDIO_IN_SIZE = 0;
    *REG_FFVM_AUDIO_IN_FMT  = 0;
    *REG_FFVM_DISP_WH       = 0;

    free(adev_buf);
    free(test_buf);
    return 0;
}
