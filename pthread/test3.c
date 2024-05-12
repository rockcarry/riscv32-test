#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fftask.h"

static int      s_exit  = 0;
static uint32_t s_count = 0;
static int      s_lock  = 0;
static int      s_error = 0;
static KOBJECT *s_mutex = NULL;
static void* task1_proc(void *arg)
{
    printf("task1_proc start ...\n");
    while (!s_exit) {
        int ret = mutex_trylock(s_mutex);
        if (ret == 0) {
            if (s_lock != 0) s_error++;
            s_lock++;
            printf("task1 mutex try lock ok\n");
            printf("task1 counter: %lu\n", s_count += 1);
            printf("task1 mutex unlock\n");
            s_lock--;
            mutex_unlock(s_mutex);
        } else {
            printf("task1 mutex try lock failed\n");
        }
        task_sleep(100);
    }
    printf("task1_proc finish !\n");
    return NULL;
}

static void* task2_proc(void *arg)
{
    printf("task2_proc start ...\n");
    while (!s_exit) {
        int ret = mutex_timedlock(s_mutex, 1000);
        if (ret == 0) {
            if (s_lock != 0) s_error++;
            s_lock++;
            printf("task2 mutex timedlock ok\n");
            printf("task2 counter: %lu\n", s_count += 1);
            task_sleep(100);
            printf("task2 mutex unlock\n");
            s_lock--;
            mutex_unlock(s_mutex);
        } else {
            printf("task2 mutex timedlock failed\n");
        }
    }
    printf("task2_proc finish !\n");
    return NULL;
}

static void* task3_proc(void *arg)
{
    printf("task3_proc start ...\n");
    while (!s_exit) {
        mutex_lock(s_mutex);
        if (s_lock != 0) s_error++;
        s_lock++;
        printf("task3 mutex lock\n");
        printf("task3 counter: %lu\n", s_count += 1);
        task_sleep(2000);
        printf("task3 mutex unlock\n");
        s_lock--;
        mutex_unlock(s_mutex);
        task_sleep(100);
    }
    printf("task3_proc finish !\n");
    return NULL;
}

static void* task4_proc(void *arg)
{
    printf("task4_proc start ...\n");
    while (!s_exit) {
        int ret = mutex_unlock(s_mutex);
        if (ret == 0) {
            s_error++;
            printf("unexpect mutex_unlock result: %d != -1\n", ret);
        }
        task_sleep(100);
    }
    printf("task4_proc finish !\n");
    return NULL;
}

int main(void)
{
    task_kernel_init();
    s_mutex = mutex_init("mutex");
    KOBJECT *task1 = task_create("task1", task1_proc, NULL, 0, 0);
    KOBJECT *task2 = task_create("task2", task2_proc, NULL, 0, 0);
    KOBJECT *task3 = task_create("task3", task3_proc, NULL, 0, 0);
    KOBJECT *task4 = task_create("task4", task4_proc, NULL, 0, 0);
    for (int i = 0; i < 10; i++) {
        task_kernel_dump("cpu usage", "stat", 1);
        task_sleep(2000);
    }
    s_exit = 1;
    task_join(task1, NULL);
    task_join(task2, NULL);
    task_join(task3, NULL);
    task_join(task4, NULL);
    mutex_destroy(s_mutex);
    task_kernel_dump("main", "all", 1);
    task_kernel_exit();
    printf("s_error: %d, test result: %s\n", s_error, s_error == 0 ? "pass" : "failed");
    return 0;
}
