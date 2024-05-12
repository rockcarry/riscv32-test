#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libffvm.h"
#include "fftask.h"

static KOBJECT *s_mutex = NULL;
static KOBJECT *s_cond  = NULL;
static int s_total_produce = 0;
static int s_total_consume = 0;
static int s_queue_max     = 10;
static int s_queue_cur     = 0;
static int s_exit          = 0;

#define PRODUCER_SLEEP_MIN  100
#define CONSUMER_SLEEP_MIN  20

static void* producer_proc(void *arg)
{
    printf("producer_proc start ...\n");
    while (!s_exit) {
        int delay = rand() % 5;
        int sleep = PRODUCER_SLEEP_MIN + rand() % 91;
        int wait  = rand() & 1 ? rand() % 50 : 0;
        int ret;
        printf("producer_proc delay: %d, sleep: %d, wait: %d\n", delay, sleep, wait);
        mutex_lock(s_mutex);
        printf("producer_proc lock\n");
        while (!s_exit && s_queue_cur >= s_queue_max) {
            printf("producer_proc cond_wait type: %s ++\n", wait ? "timed" : "infinite");
            if (!wait) ret = cond_wait(s_cond, s_mutex);
            else ret = cond_timedwait(s_cond, s_mutex, wait);
            printf("producer_proc cond_wait ret: %d --\n", ret);
        }
        if (s_queue_cur < s_queue_max) { s_total_produce++; s_queue_cur++; }
        printf("producer_proc s_total_produce: %d, s_queue_cur: %d\n", s_total_produce, s_queue_cur);
        mdelay(delay);
        cond_signal(s_cond, 0);
        printf("producer_proc unlock\n\n");
        mutex_unlock(s_mutex);
        if (sleep > 0) task_sleep(sleep);
    }
    printf("producer_proc finish !\n");
    return NULL;
}

static void* consumer_proc(void *arg)
{
    printf("consumer_proc start ...\n");
    while (!s_exit || s_queue_cur > 0) {
        int delay = rand() % 5;
        int sleep = CONSUMER_SLEEP_MIN + rand() % 91;
        int wait  = rand() & 1 ? rand() % 50 : 0;
        int ret;
        printf("consumer_proc delay: %d, sleep: %d, wait: %d\n", delay, sleep, wait);
        mutex_lock(s_mutex);
        printf("consumer_proc lock\n");
        while (!s_exit && s_queue_cur <= 0) {
            printf("consumer_proc cond_wait type: %s ++\n", wait ? "timed" : "infinite");
            if (!wait) ret = cond_wait(s_cond, s_mutex);
            else ret = cond_timedwait(s_cond, s_mutex, wait);
            printf("consumer_proc cond_wait ret: %d --\n", ret);
        }
        if (s_queue_cur > 0) { s_total_consume++; s_queue_cur--; }
        printf("consumer_proc s_total_consume: %d, s_queue_cur: %d\n", s_total_consume, s_queue_cur);
        mdelay(delay);
        cond_signal(s_cond, 0);
        printf("consumer_proc unlock\n\n");
        mutex_unlock(s_mutex);
        if (sleep > 0) task_sleep(sleep);
    }
    printf("consumer_proc finish !\n");
    return NULL;
}

int main(void)
{
    task_kernel_init();
    s_mutex = mutex_init("mutex");
    s_cond  = cond_init ("cond" );
    KOBJECT *task1 = task_create("task1", producer_proc, NULL, 0, 0);
    KOBJECT *task2 = task_create("task2", consumer_proc, NULL, 0, 0);
    for (int i = 0; i < 10; i++) {
//      task_kernel_dump("cpu usage", "stat", 1);
        task_sleep(2000);
    }
    mutex_lock(s_mutex);
    s_exit = 1;
    cond_signal(s_cond, 1);
    mutex_unlock(s_mutex);
    task_join(task1, NULL);
    task_join(task2, NULL);
    mutex_destroy(s_mutex);
    cond_destroy (s_cond);
//  task_kernel_dump("main", "all", 1);
    task_kernel_exit();
    printf("test result: %s\n", s_total_produce == s_total_consume ? "pass" : "failed");
    return 0;
}
