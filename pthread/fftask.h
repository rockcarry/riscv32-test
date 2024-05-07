#ifndef _FFTASK_H_
#define _FFTASK_H_

#include <stdint.h>

typedef struct {
    uint32_t pc;
    uint32_t ra, sp, gp, tp;
    uint32_t t0, t1, t2;
    uint32_t s0, s1;
    uint32_t a0, a1, a2, a3, a4, a5, a6, a7;
    uint32_t s2, s3, s4, s5, s6, s7, s8, s9, s10, s11;
    uint32_t t3, t4, t5, t6;

    uint32_t timeout, exitcode;
    void*  (*taskproc)(void*);
    void    *taskarg;
} TASKCTX;

typedef struct tagKOBJECT {
    struct tagKOBJECT *t_prev, *t_next;
    struct tagKOBJECT *w_prev, *w_next;

    #define FFTASK_KOBJ_TASK  0
    #define FFTASK_KOBJ_MUTEX 1
    #define FFTASK_KOBJ_COND  2
    #define FFTASK_KOBJ_SEM   3
    int      type;

    #define FFTASK_KOBJ_DEAD   (1 << 0)
    #define FFTASK_KOBJ_DETACH (1 << 1)
    uint32_t flags;

    TASKCTX *taskctx;

    union {
        struct {
            struct tagKOBJECT *joiner;
        } task;
        struct {
            struct tagKOBJECT *onwer;
        } mutex;
    };
} KOBJECT;

void     task_timer_isr(void);
TASKCTX* task_timer_schedule(void);
void     task_switch_then_interrupt_on(TASKCTX *taskctx);

void     task_kernel_init(void);
void     task_kernel_exit(void);

KOBJECT* task_create(void* (*taskproc)(void*), void *taskarg, int stacksize, int params);
int      task_join  (KOBJECT *task, uint32_t *exitcode);
int      task_detach(KOBJECT *task);
void     task_sleep (int ms);

KOBJECT* mutex_create   (void);
int      mutex_destroy  (KOBJECT *mutex);
int      mutex_lock     (KOBJECT *mutex);
int      mutex_unlock   (KOBJECT *mutex);
int      mutex_trylock  (KOBJECT *mutex);
int      mutex_timedlock(KOBJECT *mutex, int ms);

#endif
