#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "ffvmreg.h"
#include "libffvm.h"

#define AUDIO_ADEV_BUFSIZE 8129
#define AUDIO_SAMPLERATE   16000
#define AUDIO_CLIP_SIZE    64
#define AUDIO_START_FREQ   100
#define AUDIO_STOP_FREQ    8000
#define AUDIO_FFT_LEN      1024
#define AUDIO_FREQ_TO_IDX(freq) ((freq) * AUDIO_FFT_LEN / AUDIO_SAMPLERATE)
#define AUDIO_IDX_TO_FREQ(idx ) ( AUDIO_SAMPLERATE * (idx) / AUDIO_FFT_LEN)

float gen_sin_pcm_with_phase(int16_t *pcm, int n, int samprate, int freq, int amp, float phase)
{
    int i; for (i = 0; i < n; i++) pcm[i] = amp * sin(phase + i * 2 * M_PI * freq / samprate);
    return phase + i * 2 * M_PI * freq / samprate;
}

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

int main(void)
{
    int      start_idx = AUDIO_FREQ_TO_IDX(AUDIO_START_FREQ);
    int      stop_idx  = AUDIO_FREQ_TO_IDX(AUDIO_STOP_FREQ );
    int      test_len  = AUDIO_CLIP_SIZE * (stop_idx - start_idx + 1) * sizeof(int16_t);
    int16_t *test_buf  = malloc(test_len);
    int16_t *ptr       = test_buf;
    float    phase     = 0;
    int      i;
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
        int tail = *REG_FFVM_AUDIO_OUT_TAIL;
        int size = *REG_FFVM_AUDIO_OUT_SIZE;
        int curr = *REG_FFVM_AUDIO_OUT_CURR;
        i = size - curr;
        i = i < src_len ? i : src_len;
        if (i > 0) {
            tail     = ringbuf_write(adev_buf, size, tail, src_buf, i);
            curr    += i;
            src_len -= i;
            src_buf += i;
            *REG_FFVM_AUDIO_OUT_LOCK = 1;
            *REG_FFVM_AUDIO_OUT_TAIL = tail;
            *REG_FFVM_AUDIO_OUT_CURR = curr;
            *REG_FFVM_AUDIO_OUT_LOCK = 0;
        }
    }
    *REG_FFVM_AUDIO_OUT_SIZE = 0;
    *REG_FFVM_AUDIO_OUT_FMT  = 0;

    free(adev_buf);
    free(test_buf);
    return 0;
}
