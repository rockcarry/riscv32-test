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
    uint64_t mtimecur   = ((*REG_FFVM_MTIMECURL << 0) | ((uint64_t)*REG_FFVM_MTIMECURH << 32)) + 20; // 50Hz freqency
    *REG_FFVM_MTIMECMPL = mtimecur >>  0;
    *REG_FFVM_MTIMECMPH = mtimecur >> 32;
    s_old_mtvec = csr_read(RISCV_CSR_MTVEC);
    csr_write(RISCV_CSR_MTVEC, (uint32_t)task_timer_isr); // setup timer isr
    csr_setbits(RISCV_CSR_MIE, (1 << 7)); // enable timer interrupt
}

static void timer_interrupt_done(void)
{
    uint64_t mtimecmp   = ((*REG_FFVM_MTIMECMPL << 0) | ((uint64_t)*REG_FFVM_MTIMECMPH << 32)) + 20; // 50Hz freqency
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

static void* idle_proc(void *arg)
{
    while (1) *REG_FFVM_CPU_FREQ = 10 * 1000 * 1000;
    return NULL;
}

static TASKCTX* task_schedule(KOBJECT *obj, int broadcast)
{
    KOBJECT *k, *t;
    if (!obj) {
        uint32_t tick = get_tick_count();
        for (k = s_sleep_queue.t_next; k != &s_sleep_queue; ) {
            t = k; k = k->t_next;
            if ((int32_t)tick - (int32_t)t->taskctx->timeout >= 0) { w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t); }
        }
        s_running_task = s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_TASK) {
        s_running_task = obj->task.joiner ? obj->task.joiner : s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_MUTEX) {
        obj->mutex.onwer = obj->w_next != obj ? obj->w_next : NULL;
        s_running_task = obj->w_next != obj ? obj->w_next : s_ready_queue.t_next;
    }
    if (s_running_task == &s_ready_queue) s_running_task = s_idle_task;
    w_dequeue(s_running_task); t_dequeue(s_running_task);
    return s_running_task->taskctx;
}

static void task_entry(KOBJECT *task)
{
    TASKCTX *taskctx = NULL;
    task->taskctx->exitcode = (uint32_t)task->taskctx->taskproc(task->taskctx->taskarg);
    interrupt_off();
    if (task->flags & FFTASK_KOBJ_DETACH) {
        free(task);
        taskctx = task_schedule(NULL, 0);
    } else {
        task->flags |= FFTASK_KOBJ_DEAD;
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
    s_ready_queue.w_next = s_ready_queue.w_prev = &s_ready_queue;
    s_sleep_queue.t_next = s_sleep_queue.t_prev = &s_sleep_queue;
    s_sleep_queue.w_next = s_sleep_queue.w_prev = &s_sleep_queue;

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
    timer_interrupt_done();
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
    if (!task || task->type != FFTASK_KOBJ_TASK || (task->flags & FFTASK_KOBJ_DETACH) || task == s_running_task || task->task.joiner) { interrupt_on(); return -1; }
    if (!(task->flags & FFTASK_KOBJ_DEAD)) {
        task->task.joiner = s_running_task;
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
    }
    if (exitcode) *exitcode = task->taskctx->exitcode;
    free(task);
    return 0;
}

int task_detach(KOBJECT *task)
{
    int ret = 0;
    interrupt_off();
    if (!task || task->type != FFTASK_KOBJ_TASK || task->w_next) ret = -1;
    if (task->flags & FFTASK_KOBJ_DEAD) free(task);
    else task->flags |= FFTASK_KOBJ_DETACH;
    interrupt_on();
    return ret;
}

void task_sleep(int ms)
{
    interrupt_off();
    s_running_task->taskctx->timeout = get_tick_count() + ms;
    t_enqueue(&s_sleep_queue, s_running_task);
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
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.onwer != s_running_task) { interrupt_on(); return -1; }
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

int mutex_timedlock(KOBJECT *mutex, int ms)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (mutex->mutex.onwer) {
        s_running_task->taskctx->timeout = get_tick_count() + ms;
        t_enqueue(&s_sleep_queue, s_running_task); w_enqueue(mutex, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
        return mutex->mutex.onwer == s_running_task ? 0 : -1;
    } else {
        mutex->mutex.onwer = s_running_task;
        interrupt_on();
        return 0;
    }
}
