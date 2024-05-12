#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "ffvmreg.h"
#include "libffvm.h"
#include "fftask.h"

typedef struct {
    char     name[8];
    KOBJECT *htask;
    uint32_t ecode0;
    uint32_t ecode1;
    int loop, detach, diff, delay_time, sleep_time;
    int detach_result;
} TASKTEST;

static void* task_proc(void *arg)
{
    TASKTEST *test = arg;
    uint32_t  ticks[100];
    int       i, d;

    printf("%s start running, loop: %3d, detach: %d, delay: %d, sleep: %d ...\n", test->name, test->loop, test->detach, test->delay_time, test->sleep_time);
    if (test->detach == 2) test->detach_result = task_detach(task_self());

    for (i = 0; i < test->loop; i++) {
        ticks[i] = *REG_FFVM_MTIMECURL;
        if (i > 1) {
            d = abs(ticks[i] - ticks[i - 1] - test->delay_time - test->sleep_time);
            if (test->diff < d) test->diff = d;
        }
        mdelay    (test->delay_time);
        task_sleep(test->sleep_time);
    }

    printf("%s finish !\n", test->name);
    if (test->detach) test->ecode1 = test->ecode0;
    return (void*)test->ecode0;
}

static int s_exit = 0;
static void* cpu_usage_proc(void *arg)
{
    while (!s_exit) {
        task_kernel_dump("cpu usage", "stat", 1);
        task_sleep(2000);
    }
    return NULL;
}

int main(void)
{
    TASKTEST *all_task_tests = calloc(100, sizeof(TASKTEST));
    int  task_num = 20, i;

    srand(time(NULL));
    task_kernel_init();
    KOBJECT *cpumon = task_create("cpumon", cpu_usage_proc, NULL, 0, 0);

    for (i = 0; i < task_num; i++) {
        sprintf(all_task_tests[i].name, "task%02d", i);
        all_task_tests[i].ecode0 = rand();
        all_task_tests[i].ecode1 = all_task_tests[i].ecode0 + 1;
        all_task_tests[i].loop   = 10 + rand() % 91;
        all_task_tests[i].detach = rand() % 3;
        all_task_tests[i].delay_time = 0  + rand() % 3;
        all_task_tests[i].sleep_time = 20 + rand() % 981;

        all_task_tests[i].htask = task_create(all_task_tests[i].name, task_proc, all_task_tests + i, 0, 0);
        if (all_task_tests[i].detach == 1) all_task_tests[i].detach_result = task_detach(all_task_tests[i].htask);
    }

    for (i = 0; i < task_num; i++) {
        if (all_task_tests[i].detach == 0) {
            int ret  = task_join(all_task_tests[i].htask, &all_task_tests[i].ecode1);
            int pass = ret == 0 && all_task_tests[i].ecode0 == all_task_tests[i].ecode1 && all_task_tests[i].diff < task_num * 10;;
            printf("%s join, ret: %d, exit code: %lu, diff: %d, %s !\n", all_task_tests[i].name, ret, all_task_tests[i].ecode1, all_task_tests[i].diff, pass ? "pass" : "failed");
        } else {
            printf("%s detached, we need wait it exit ...\n", all_task_tests[i].name);
            while (all_task_tests[i].ecode0 != all_task_tests[i].ecode1) task_sleep(100);
            int pass = all_task_tests[i].ecode0 == all_task_tests[i].ecode1 && all_task_tests[i].diff < task_num * 10;
            printf("%s wait detach ok, exit code: %lu, diff: %d, %s !\n", all_task_tests[i].name, all_task_tests[i].ecode1, all_task_tests[i].diff, pass ? "pass" : "failed");
        }
    }

    s_exit = 1;
    task_join(cpumon, NULL);
    task_kernel_dump("main", "all", 1);
    free(all_task_tests);
    task_kernel_exit();
    return 0;
}
