#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "libffvm.h"
#include "fftask.h"

static KOBJECT *s_sem  = NULL;
static int      s_exit = 0;
static int s_total_produce = 0;
static int s_total_consume = 0;

#define PRODUCER_SLEEP_MIN  200
#define CONSUMER_SLEEP_MIN  20
#define SEMAPHORE_MAX_VAL   100

static void* producer_proc(void *arg)
{
    printf("producer_proc start ...\n");
    while (!s_exit) {
        int sleep = PRODUCER_SLEEP_MIN + rand() % 91;
        int num   = 1 + rand() % 5, val, ret;
        printf("producer_proc sleep: %d, num: %d\n", sleep, num);
        ret = semaphore_getvalue(s_sem, &val);
        printf("producer_proc semaphore_getvalue, ret: %d, val: %d\n", ret, val);
        if (val + num <= SEMAPHORE_MAX_VAL) {
            ret = semaphore_post(s_sem, num);
            if (ret == 0) s_total_produce += num;
            printf("producer_proc semaphore_post, ret: %d, s_total_produce: %d\n", ret, s_total_produce);
        }
        if (sleep > 0) task_sleep(sleep);
    }
    printf("producer_proc finish !\n");
    return NULL;
}

static void* consumer_proc(void *arg)
{
    printf("consumer_proc start ...\n");
    while (!s_exit) {
        int sleep    = CONSUMER_SLEEP_MIN + rand() % 91;
        int waittype = rand() % 3, ret = -1, val;
        int waittime = waittype == 0 ? -1 : waittype == 2 ? 0 : rand() % 50;
        switch (waittype) {
        case 0: // infinite
            ret = semaphore_wait(s_sem);
            break;
        case 1: // timed
            ret = semaphore_timedwait(s_sem, waittime);
            break;
        case 2: // try
            ret = semaphore_trywait(s_sem);
            break;
        }
        printf("consumer_proc sleep: %d, waittype: %d, waittime: %d, ret: %d\n", sleep, waittype, waittime, ret);
        if (ret == 0) {
            s_total_consume += 1;
            printf("consumer_proc s_total_consume: %d\n", s_total_consume);
        }
        semaphore_getvalue(s_sem, &val);
        printf("consumer_proc sem val: %d\n", val);
        if (sleep > 0) task_sleep(sleep);
    }
    while (semaphore_trywait(s_sem) == 0) s_total_consume++;
    printf("consumer_proc finish !\n");
    return NULL;
}

int main(void)
{
    task_kernel_init();
    s_sem = semaphore_init("sem", 0);
    KOBJECT *task1 = task_create("task1", producer_proc, NULL, 0, 0);
    KOBJECT *task2 = task_create("task2", consumer_proc, NULL, 0, 0);
    for (int i = 0; i < 10; i++) {
//      task_kernel_dump("cpu usage", "stat", 1);
        task_sleep(2000);
    }
    s_exit = 1;
    semaphore_post(s_sem, 1);
    s_total_produce += 1;
    task_join(task1, NULL);
    task_join(task2, NULL);
    semaphore_destroy(s_sem);
//  task_kernel_dump("main", "all", 1);
    task_kernel_exit();
    printf("test result: %s, s_total_produce: %d, s_total_consume: %d\n", s_total_produce == s_total_consume ? "pass" : "failed", s_total_produce, s_total_consume);
    return 0;
}
