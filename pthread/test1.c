#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ffvmreg.h"
#include "fftask.h"

static int s_test_task_arg = 0;
static int s_test_task_run = 1;
static int s_test_task_exit= 0;
static int s_test_task_join= 0;
static int s_test_task_main= 1;

static void* task1_proc(void *arg)
{
    uint32_t ticks[100];
    char *hello = arg;
    int   i, d;

    if (strcmp(hello, "hello fftask !") == 0) s_test_task_arg = 1;
    printf("%s\n", hello);

    for (i = 0; i < 100; i++) {
        ticks[i] = *REG_FFVM_MTIMECURL;
        printf("%d task1, cur_tick: %lu\n", i, ticks[i]);
        task_sleep(200);
    }

    for (i = 1; i < 100; i++) {
        d = abs(ticks[i] - ticks[i - 1] - 200);
        if (d > 20) { s_test_task_run = 0; break; }
    }

    return (void*)12345678;
}

int main(void)
{
    char *hello = "hello fftask !";
    int   exitcode = 0, d = 0, ret, i;
    static char *strtab[2] = { "failed", "pass" };

    uint32_t ticks[10];
    task_kernel_init();
    KOBJECT *task1 = task_create("task1", task1_proc, hello, 0, 0);
    for (i = 0; i < 10; i++) {
//      task_kernel_dump("main", "stat", 1);
        ticks[i] = *REG_FFVM_MTIMECURL;
        if (i > 0) {
            d = abs(ticks[i] - ticks[i - 1] - 1000);
            if (d > 20) s_test_task_main = 0;
        }
        printf("%d main task, cur_tick: %lu, diff: %d\n", i, ticks[i], d);
        task_sleep(1000);
    }
    ret = task_join(task1, (uint32_t*)&exitcode);
    task_kernel_exit();

    printf("exitcode: %d\n", exitcode);
    printf("join ret: %d\n", ret);
    s_test_task_exit = exitcode == 12345678;
    s_test_task_join = ret == 0;

    printf("s_test_task_arg : %s\n", strtab[s_test_task_arg ]);
    printf("s_test_task_run : %s\n", strtab[s_test_task_run ]);
    printf("s_test_task_exit: %s\n", strtab[s_test_task_exit]);
    printf("s_test_task_join: %s\n", strtab[s_test_task_join]);
    printf("s_test_task_main: %s\n", strtab[s_test_task_main]);
    return 0;
}
