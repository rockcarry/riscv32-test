#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ffvmreg.h"
#include "libffvm.h"
#include "fftask.h"
#include "ethphy.h"

typedef struct {
    #define FLAG_EXIT (1 << 0)
    uint32_t  flags;
    KOBJECT  *task;
    uint8_t   buf[64 * 1024];
    PFN_ETHPHY_CALLBACK callback;
    void               *cbctx;
} PHYDEV;

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

static void* ethphy_work_proc(void *arg)
{
    PHYDEV  *phy = arg;
    uint8_t  buf[2048], *pkt;
    uint32_t len = 0, head;
    while (!(phy->flags & FLAG_EXIT)) {
        semaphore_wait(g_sem_irq_eth);

        len = 0;
        *REG_FFVM_ETHPHY_IN_LOCK = 1;
        while (*REG_FFVM_ETHPHY_IN_CURR >= sizeof(uint32_t)) {
            head = ringbuf_read(phy->buf, *REG_FFVM_ETHPHY_IN_SIZE, *REG_FFVM_ETHPHY_IN_HEAD, (uint8_t*)&len, sizeof(len));
            if (*REG_FFVM_ETHPHY_IN_CURR < sizeof(len) + len) { len = 0; break; }

            if (len > sizeof(buf)) printf("ethphy input packet size %lu too big drop it !\n", len);
            pkt = len > sizeof(buf) ? NULL : buf;

            head = ringbuf_read(phy->buf, *REG_FFVM_ETHPHY_IN_SIZE, head, pkt, len);
            *REG_FFVM_ETHPHY_IN_HEAD  = head;
            *REG_FFVM_ETHPHY_IN_CURR -= sizeof(len) + len;
        }
        *REG_FFVM_ETHPHY_IN_LOCK = 0;
        if (phy->callback && len) phy->callback(phy->cbctx, (char*)buf, len);
    }
    return NULL;
}

void* ethphy_open(int dev, PFN_ETHPHY_CALLBACK callback, void *cbctx)
{
    PHYDEV *phy = calloc(1, sizeof(PHYDEV));
    if (!phy) return NULL;
    *REG_FFVM_ETHPHY_IN_ADDR = (uint32_t)phy->buf;
    *REG_FFVM_ETHPHY_IN_SIZE = (uint32_t)sizeof(phy->buf);
    *REG_FFVM_IRQ_ENABLE    |= FLAG_FFVM_IRQ_ETHPHY;
    phy->callback = callback;
    phy->cbctx    = cbctx;
    phy->task     = task_create(NULL, ethphy_work_proc, phy, 0, 0);
    return phy;
}

void ethphy_close(void *ctx)
{
    if (!ctx) return;
    PHYDEV *phy = ctx;
    phy->flags |= FLAG_EXIT;
    semaphore_post(g_sem_irq_eth, 1);
    task_join(phy->task, NULL);
    *REG_FFVM_IRQ_ENABLE    &= ~FLAG_FFVM_IRQ_ETHPHY;
    *REG_FFVM_ETHPHY_IN_SIZE =  0;
    free(phy);
}

int ethphy_send(void *ctx, char *buf, int len)
{
    if (!ctx) return -1;
    *REG_FFVM_ETHPHY_OUT_ADDR = (uint32_t)buf;
    *REG_FFVM_ETHPHY_OUT_SIZE = (uint32_t)len;
    return 0;
}

#ifdef _TEST_
static void my_ethphy_callback(void *cbctx, char *buf, int len)
{
    printf("ethphy get input packet buf: %p, len: %d\n", buf, len);
}

int main(void)
{
    task_kernel_init();
    void *phy = ethphy_open(0, my_ethphy_callback, NULL);
    while (1) {
        if (kbhit()) {
            char c = getch();
            if (c == 'q' || c == 'Q') break;
        }
        task_sleep(20);
    }
    ethphy_close(phy);
    task_kernel_exit();
    return 0;
}
#endif
