#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "ffvmreg.h"
#include "libffvm.h"
#include "fftask.h"

static uint32_t s_old_mtvec    = 0;
static uint32_t s_old_mscratch = 0;

static uint32_t csr_read(uint32_t addr)
{
    uint32_t value;
    asm volatile ( "csrr %0, %1" : "=r"(value) : "i"(addr) );
    return value;
}

static void csr_write(uint32_t addr, uint32_t value) {
    asm volatile ( "csrw %0, %1" :: "i" (addr), "r" (value) );
}

static void csr_setbits(uint32_t addr, uint32_t bits)
{
    asm volatile ( "csrs %0, %1" :: "i" (addr), "r" (bits) );
}

static void csr_clrbits(uint32_t addr, uint32_t bits)
{
    asm volatile ( "csrc %0, %1" :: "i" (addr), "r" (bits) );
}

static void timer_interrupt_init(void)
{
    uint64_t mtimecmp   = ((*REG_FFVM_MTIMECURL << 0) | ((uint64_t)*REG_FFVM_MTIMECURH << 32)) + 20; // 50Hz freqency
    *REG_FFVM_MTIMECMPL = mtimecmp >>  0;
    *REG_FFVM_MTIMECMPH = mtimecmp >> 32;
    s_old_mtvec = csr_read(RISCV_CSR_MTVEC);
    csr_write(RISCV_CSR_MTVEC, (uint32_t)task_timer_isr); // setup timer isr
    csr_setbits(RISCV_CSR_MIE, (1 << 7)); // enable timer interrupt
}

static void timer_interrupt_next(int timeout)
{
    uint64_t mtimecmp   = ((*REG_FFVM_MTIMECURL << 0) | ((uint64_t)*REG_FFVM_MTIMECURH << 32)) + timeout;
    *REG_FFVM_MTIMECMPL = mtimecmp >>  0;
    *REG_FFVM_MTIMECMPH = mtimecmp >> 32;
}

static void timer_interrupt_exit(void)
{
    *REG_FFVM_MTIMECMPL = 0xFFFFFFFF;
    *REG_FFVM_MTIMECMPH = 0xFFFFFFFF;
    csr_write(RISCV_CSR_MTVEC, s_old_mtvec); // restore old timer isr
    csr_clrbits(RISCV_CSR_MIE, (1 << 7)); // disable timer interrupt
}

static void interrupt_on (void) { csr_setbits(RISCV_CSR_MSTATUS, (1 << 3)); }
static void interrupt_off(void) { csr_clrbits(RISCV_CSR_MSTATUS, (1 << 3)); }

static KOBJECT  s_ready_queue  = {};
static KOBJECT  s_sleep_queue  = {};
static KOBJECT *s_main_task    = NULL;
static KOBJECT *s_idle_task    = NULL;
static KOBJECT *s_running_task = NULL;

static uint32_t get_tick_count(void)
{
    return *REG_FFVM_MTIMECURL;
}

static void t_enqueue(KOBJECT *queue, KOBJECT *obj)
{
    obj->t_prev = queue->t_prev;
    obj->t_next = queue;
    queue->t_prev->t_next = obj;
    queue->t_prev = obj;
}

static void t_dequeue(KOBJECT *obj)
{
    obj->t_prev->t_next = obj->t_next;
    obj->t_next->t_prev = obj->t_prev;
    obj->t_next = obj->t_prev = obj;
}

static void w_enqueue(KOBJECT *queue, KOBJECT *obj)
{
    obj->w_prev = queue->w_prev;
    obj->w_next = queue;
    queue->w_prev->w_next = obj;
    queue->w_prev = obj;
}

static void w_dequeue(KOBJECT *obj)
{
    obj->w_prev->w_next = obj->w_next;
    obj->w_next->w_prev = obj->w_prev;
    obj->w_next = obj->w_prev = obj;
}

static void s_insert(KOBJECT *obj)
{
    KOBJECT *k, *t;
    obj->taskctx->flags &= ~FFTASK_TASK_TIMEOUT;
    for (k = s_sleep_queue.t_next; k != &s_sleep_queue && (t = k); k = k->t_next) {
        if (obj->taskctx->timeout < t->taskctx->timeout) {
            obj->t_prev = t->t_prev;
            obj->t_next = t;
            t->t_prev->t_next = obj;
            t->t_prev = obj;
            break;
        }
    }
}

static void* idle_proc(void *arg)
{
    while (1) {}
    return NULL;
}

static TASKCTX* task_schedule(KOBJECT *obj, int arg)
{
    uint32_t tick = get_tick_count();
    int32_t  next;
    KOBJECT *k, *t;
    if (!obj) {
        for (k = s_sleep_queue.t_next; k != &s_sleep_queue && (int32_t)tick - (int32_t)k->taskctx->timeout >= 0 && (t = k); k = k->t_next) {
            t->taskctx->flags |= FFTASK_TASK_TIMEOUT; w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t);
        }
        s_running_task = s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_TASK) {
        s_running_task = obj->task.joiner ? obj->task.joiner : s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_MUTEX) {
        obj->mutex.onwer = obj->w_next != obj ? obj->w_next : NULL;
        s_running_task   = obj->w_next != obj ? obj->w_next : s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_COND) {
        s_running_task   = obj->w_next != obj ? obj->w_next : s_ready_queue.t_next;
        if (arg) { // broadcase
            for (k = obj->w_next->w_next; k != obj && (t = k); k = k->w_next) { w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t); }
        }
    } else if (obj->type == FFTASK_KOBJ_SEM) {
        if (obj->w_next != obj) {
            s_running_task = obj->w_next;
            for (arg--, k = obj->w_next->w_next; k != obj && arg && (t = k); k = k->w_next, arg--) { w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t); }
        } else {
            s_running_task = s_ready_queue.t_next;
        }
        obj->sem.val += arg;
    }
    if (s_running_task == &s_ready_queue) {
        s_running_task = s_idle_task;
        next = (int32_t)s_sleep_queue.t_next - (int32_t)get_tick_count();
        *REG_FFVM_CPU_FREQ = 100 * 1000; // 100KHz cpu freq
        timer_interrupt_next(next > 1 ? next : 1);
    } else {
        *REG_FFVM_CPU_FREQ = 0xFFFFFFFF; // switch to max cpu freq
        timer_interrupt_next(20); // next 20ms trigger timer interrupt
    }
    w_dequeue(s_running_task); t_dequeue(s_running_task);
    return s_running_task->taskctx;
}

static void task_entry(KOBJECT *task)
{
    TASKCTX *taskctx = NULL;
    task->taskctx->exitcode = (uint32_t)task->taskctx->taskproc(task->taskctx->taskarg);
    interrupt_off();
    if (task->taskctx->flags & FFTASK_TASK_DETACH) {
        free(task);
        taskctx = task_schedule(NULL, 0);
    } else {
        task->taskctx->flags |= FFTASK_TASK_DEAD;
        taskctx = task_schedule(task, 0);
    }
    task_switch_then_interrupt_on(taskctx);
}

static KOBJECT* task_create_internal(void* (*taskproc)(void*), void *taskarg, int stacksize)
{    KOBJECT *obj = malloc(sizeof(KOBJECT) + sizeof(TASKCTX) + stacksize);
    if (!obj) return NULL;
    memset(obj, 0, sizeof(KOBJECT) + sizeof(TASKCTX));
    register uint32_t gp asm("gp");
    obj->t_next = obj->t_prev = obj;
    obj->w_next = obj->w_prev = obj;
    obj->taskctx           = (TASKCTX*)(obj + 1);
    obj->taskctx->pc       = (uint32_t)task_entry;
    obj->taskctx->ra       = (uint32_t)task_entry;
    obj->taskctx->a0       = (uint32_t)obj;
    obj->taskctx->sp       = stacksize ? (uint32_t)((uint8_t*)(obj->taskctx + 1) + stacksize) : 0;
    obj->taskctx->gp       = gp;
    obj->taskctx->taskproc = taskproc;
    obj->taskctx->taskarg  = taskarg;
    return obj;
}

void task_kernel_init(void)
{
    interrupt_off();

    timer_interrupt_init();

    s_ready_queue.t_next = s_ready_queue.t_prev = &s_ready_queue;
    s_sleep_queue.t_next = s_sleep_queue.t_prev = &s_sleep_queue;

    s_idle_task    = task_create_internal(idle_proc, NULL, 256);
    s_main_task    = s_running_task = task_create_internal(NULL, NULL, 0);
    s_old_mscratch = csr_read(RISCV_CSR_MSCRATCH);
    csr_write(RISCV_CSR_MSCRATCH, (uint32_t)s_main_task->taskctx);

    interrupt_on();
}

void task_kernel_exit(void)
{
    KOBJECT *obj, *tmp;
    interrupt_off();
    csr_write(RISCV_CSR_MSCRATCH, s_old_mscratch);
    for (obj = s_ready_queue.t_next; obj != &s_ready_queue; ) { tmp = obj; obj = obj->t_next; t_dequeue(tmp); free(tmp); }
    for (obj = s_sleep_queue.t_next; obj != &s_sleep_queue; ) { tmp = obj; obj = obj->t_next; t_dequeue(tmp); free(tmp); }
    s_main_task = s_idle_task = s_running_task = NULL;
    timer_interrupt_exit();
    interrupt_on();
}

TASKCTX* task_timer_schedule(void)
{
    t_enqueue(&s_ready_queue, s_running_task);
    return task_schedule(NULL, 0);
}

KOBJECT* task_create(void* (*taskfunc)(void*), void *taskarg, int stacksize, int params)
{
    if (!taskfunc) return NULL;
    interrupt_off();
    KOBJECT *obj = task_create_internal(taskfunc, taskarg, stacksize ? stacksize : (64 * 1024));
    if (obj) t_enqueue(&s_ready_queue, obj);
    interrupt_on();
    return obj;
}

int task_join(KOBJECT *task, uint32_t *exitcode)
{
    interrupt_off();
    if (!task || task->type != FFTASK_KOBJ_TASK || (task->taskctx->flags & FFTASK_TASK_DETACH) || task == s_running_task || task->task.joiner) { interrupt_on(); return -1; }
    if (!(task->taskctx->flags & FFTASK_TASK_DEAD)) {
        task->task.joiner = s_running_task;
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
    }
    if (exitcode) *exitcode = task->taskctx->exitcode;
    free(task);
    return 0;
}

int task_detach(KOBJECT *task)
{
    interrupt_off();
    int ret = (!task || task->type != FFTASK_KOBJ_TASK) ? -1 : 0;
    if (task->taskctx->flags & FFTASK_TASK_DEAD) free(task);
    else task->taskctx->flags |= FFTASK_TASK_DETACH;
    interrupt_on();
    return ret;
}

void task_sleep(int32_t ms)
{
    interrupt_off();
    s_running_task->taskctx->timeout = get_tick_count() + ms;
    s_insert(s_running_task);
    task_switch_then_interrupt_on(task_schedule(NULL, 0));
}

KOBJECT* mutex_init(void)
{
    KOBJECT *mutex = calloc(1, sizeof(KOBJECT));
    if (!mutex) return NULL;
    mutex->type = FFTASK_KOBJ_MUTEX;
    mutex->w_next = mutex->w_prev = mutex;
    return mutex;
}

int mutex_destroy(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.onwer || mutex->w_next != mutex) { interrupt_on(); return -1; }
    free(mutex);
    interrupt_on();
    return 0;
}

int mutex_lock(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (mutex->mutex.onwer) {
        w_enqueue(mutex, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
    } else {
        mutex->mutex.onwer = s_running_task;
        interrupt_on();
    }
    return 0;
}

int mutex_unlock(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.onwer != s_running_task || mutex->w_next == mutex) { interrupt_on(); return -1; }
    t_enqueue(&s_ready_queue, s_running_task);
    task_switch_then_interrupt_on(task_schedule(mutex, 0));
    return 0;
}

int mutex_trylock(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.onwer) { interrupt_on(); return -1; }
    mutex->mutex.onwer = s_running_task;
    interrupt_on();
    return 0;
}

int mutex_timedlock(KOBJECT *mutex, int32_t ms)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (mutex->mutex.onwer) {
        s_running_task->taskctx->timeout = get_tick_count() + ms;
        s_insert(s_running_task); w_enqueue(mutex, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
        return mutex->mutex.onwer == s_running_task ? 0 : -1;
    } else {
        mutex->mutex.onwer = s_running_task;
        interrupt_on();
        return 0;
    }
}

KOBJECT* cond_init(void)
{
    KOBJECT *cond = calloc(1, sizeof(KOBJECT));
    if (!cond) return NULL;
    cond->type = FFTASK_KOBJ_COND;
    cond->w_next = cond->w_prev = cond;
    return cond;
}

int cond_destroy(KOBJECT *cond)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || cond->w_next != cond) { interrupt_on(); return -1; }
    free(cond);
    interrupt_on();
    return 0;
}

int cond_wait(KOBJECT *cond, KOBJECT *mutex)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    w_enqueue(cond, s_running_task);
    task_switch_then_interrupt_on(task_schedule(mutex, 0));
    return 0;
}

int cond_timedwait(KOBJECT *cond, KOBJECT *mutex, int32_t ms)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    s_running_task->taskctx->timeout = get_tick_count() + ms;
    s_insert(s_running_task); w_enqueue(cond, s_running_task);
    task_switch_then_interrupt_on(task_schedule(mutex, 0));
    return (s_running_task->taskctx->flags & FFTASK_TASK_TIMEOUT) ? -1 : 0;
}

int cond_signal(KOBJECT *cond, int broadcast)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || cond->w_next == cond) { interrupt_on(); return -1; }
    t_enqueue(&s_ready_queue, s_running_task);
    task_switch_then_interrupt_on(task_schedule(cond, broadcast));
    return 0;
}

KOBJECT* semaphore_init(int val)
{
    KOBJECT *sem = calloc(1, sizeof(KOBJECT));
    if (!sem) return NULL;
    sem->type    = FFTASK_KOBJ_SEM;
    sem->w_next  = sem->w_prev = sem;
    sem->sem.val = val;
    return sem;
}

int semaphore_destroy(KOBJECT *sem)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_SEM || sem->w_next != sem) { interrupt_on(); return -1; }
    free(sem);
    interrupt_on();
    return 0;
}

int semaphore_trywait(KOBJECT *sem)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_SEM || sem->sem.val == 0) { interrupt_on(); return -1; }
    sem->sem.val--;
    interrupt_on();
    return 0;
}

int semaphore_wait(KOBJECT *sem)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (sem->sem.val == 0) {
        w_enqueue(sem, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
    } else {
        sem->sem.val--;
        interrupt_on();
    }
    return 0;
}

int semaphore_timedwait(KOBJECT *sem, int32_t ms)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (sem->sem.val == 0) {
        s_running_task->taskctx->timeout = get_tick_count() + ms;
        s_insert(s_running_task); w_enqueue(sem, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
        return (s_running_task->taskctx->flags & FFTASK_TASK_TIMEOUT) ? -1 : 0;
    } else {
        sem->sem.val--;
        interrupt_on();
        return 0;
    }
}

int semaphore_post(KOBJECT *sem, int n)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_MUTEX || n <= 0) { interrupt_on(); return -1; }
    t_enqueue(&s_ready_queue, s_running_task);
    task_switch_then_interrupt_on(task_schedule(sem, n));
    return 0;
}

int semaphore_getvalue(KOBJECT *sem, int *val)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (val) *val = sem->sem.val;
    interrupt_on();
    return 0;
}
