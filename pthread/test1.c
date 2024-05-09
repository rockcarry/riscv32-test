#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "fftask.h"

static int      s_exit  = 0;
static KOBJECT *s_mutex = NULL;
static void* task1_proc(void *arg)
{
    uint32_t *counter = arg;
    while (!s_exit) {
        int ret = mutex_trylock(s_mutex);
        if (ret == 0) {
            printf("task1 mutex try lock ok\n");
            printf("task 1 counter: %lu\n", *counter += 1);
            printf("task1 mutex unlock\n");
            mutex_unlock(s_mutex);
        } else {
            printf("task1 mutex try lock failed\n");
        }
        task_sleep(100);
    }
    return (void*)1;
}

static void* task2_proc(void *arg)
{
    uint32_t *counter = arg;
    while (!s_exit) {
        int ret = mutex_timedlock(s_mutex, 1000);
        if (ret == 0) {
            printf("task2 mutex timedlock ok\n");
            printf("task 2 counter: %lu\n", *counter += 1);
            printf("task2 mutex unlock\n");
            mutex_unlock(s_mutex);
        } else {
            printf("task2 mutex timedlock failed\n");
        }
    }
    return (void*)2;
}

static void* task3_proc(void *arg)
{
    uint32_t *counter = arg;
    while (!s_exit) {
        mutex_lock(s_mutex);
        printf("task3 mutex lock\n");
        printf("task 3 counter: %lu\n", *counter += 1);
        task_sleep(1000);
        printf("task3 mutex unlock\n");
        mutex_unlock(s_mutex);
    }
    return (void*)3;
}

int main(void)
{
    uint32_t counter1 = 0;
    uint32_t counter2 = 0;
    uint32_t counter3 = 0;
    uint32_t exitcode = 0;
    task_kernel_init();
    s_mutex = mutex_create();
    KOBJECT *task1 = task_create(task1_proc, &counter1, 0, 0);
    KOBJECT *task2 = task_create(task2_proc, &counter2, 0, 0);
    KOBJECT *task3 = task_create(task3_proc, &counter3, 0, 0);
    while (!s_exit) {
        char cmd[256];
        scanf("%255s", cmd);
        printf("cmd: %s\n", cmd);
        if (strcmp(cmd, "exit") == 0) { s_exit = 1; break; }
    }
    task_join(task1, &exitcode); printf("task1 exit: %lu\n", exitcode);
    task_join(task2, &exitcode); printf("task2 exit: %lu\n", exitcode);
    task_join(task3, &exitcode); printf("task3 exit: %lu\n", exitcode);
    mutex_destroy(s_mutex);
    task_kernel_exit();
    return 0;
}
