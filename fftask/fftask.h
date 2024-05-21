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

#ifdef DEBUG
    uint32_t last_run_tick;
    uint32_t last_ready_tick;
    uint32_t last_sleep_tick;
    uint32_t last_wait_tick;
    uint32_t total_run_tick;
#endif
} TASKCTX;

typedef struct tagKOBJECT {
    struct tagKOBJECT *t_prev, *t_next;
    struct tagKOBJECT *w_prev, *w_next;

    #define FFTASK_KOBJ_TASK  0
    #define FFTASK_KOBJ_MUTEX 1
    #define FFTASK_KOBJ_COND  2
    #define FFTASK_KOBJ_SEM   3
    int      type;

    #define FFTASK_TASK_DEAD      (1 << 0)
    #define FFTASK_TASK_DETACH    (1 << 1)
    #define FFTASK_TASK_TIMEOUT   (1 << 2)
    #define FFTASK_MONOTONIC_TIME (1 << 3)
    uint32_t flags;

    char     name[8];
    TASKCTX *taskctx;

    union {
        struct {
            struct tagKOBJECT *joiner;
        } task;
        struct {
            struct tagKOBJECT *owner;
        } mutex;
        struct {
            uint32_t val;
        } sem;
    };
} KOBJECT;

void     task_isr_vector(void);
TASKCTX* task_eintr_handler (void);
TASKCTX* task_timer_schedule(void);
void     task_switch_then_interrupt_on(TASKCTX *taskctx);

void     task_kernel_init(void);
void     task_kernel_exit(void);
void     task_kernel_dump(char *title, char *type, int flag);
void     task_kernel_set_eintr_callback(KOBJECT* (*callback)(void*), void *cbctx);

#define FFTASK_TASK_CREATE_DETACH (1 << 3)
KOBJECT* task_create(char *name, void* (*taskproc)(void*), void *taskarg, int stacksize, int flags);
int      task_join  (KOBJECT *task, uint32_t *exitcode);
int      task_detach(KOBJECT *task);
void     task_sleep (int32_t ms);
KOBJECT* task_self  (void);

KOBJECT* mutex_init     (char *name);
int      mutex_destroy  (KOBJECT *mutex);
int      mutex_lock     (KOBJECT *mutex);
int      mutex_unlock   (KOBJECT *mutex);
int      mutex_trylock  (KOBJECT *mutex);
int      mutex_timedlock(KOBJECT *mutex, int32_t ms);

KOBJECT* cond_init      (char *name);
int      cond_destroy   (KOBJECT *cond);
int      cond_wait      (KOBJECT *cond, KOBJECT *mutex);
int      cond_timedwait (KOBJECT *cond, KOBJECT *mutex, int32_t ms);
int      cond_signal    (KOBJECT *cond, int broadcast);

KOBJECT* semaphore_init     (char *name, int val);
int      semaphore_destroy  (KOBJECT *sem);
int      semaphore_trywait  (KOBJECT *sem);
int      semaphore_wait     (KOBJECT *sem);
int      semaphore_timedwait(KOBJECT *sem, int32_t ms);
int      semaphore_post     (KOBJECT *sem, int n);
KOBJECT* semaphore_post_isr (KOBJECT *sem);
int      semaphore_getvalue (KOBJECT *sem, int *val);

extern KOBJECT *g_sem_irq_ai ;
extern KOBJECT *g_sem_irq_ao ;
extern KOBJECT *g_sem_irq_eth;

#endif
