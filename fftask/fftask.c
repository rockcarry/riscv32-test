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
    csr_write(RISCV_CSR_MTVEC, (uint32_t)task_isr_vector + 1); // setup timer isr
    csr_setbits(RISCV_CSR_MIE    , (1 << 7) | (1 << 11)); // enable timer & external interrupt
    csr_setbits(RISCV_CSR_MSTATUS, (1 << 7)); // set mstatus:mip to 1
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
    csr_clrbits(RISCV_CSR_MIE    , (1 << 7) | (1 << 11)); // disable timer & external interrupt
    csr_clrbits(RISCV_CSR_MSTATUS, (1 << 7)); // set mstatus:mip to 0
}

static void interrupt_on (void) { csr_setbits(RISCV_CSR_MSTATUS, (1 << 3)); }
static void interrupt_off(void) { csr_clrbits(RISCV_CSR_MSTATUS, (1 << 3)); }

static KOBJECT  s_ready_queue  = {};
static KOBJECT  s_sleep_queue  = {};
static KOBJECT  s_objct_queue  = {};
static KOBJECT *s_main_task    = NULL;
static KOBJECT *s_idle_task    = NULL;
static KOBJECT *s_running_task = NULL;

static uint32_t s_stat_intr_count = 0;
static uint32_t s_stat_start_tick = 0;
static uint32_t s_stat_idle_start = 0;
static uint32_t s_stat_idle_total = 0;

static void t_enqueue(KOBJECT *queue, KOBJECT *obj)
{
#ifdef DEBUG
    if      (queue == &s_ready_queue) obj->taskctx->last_ready_tick = get_tick_count();
    else if (queue == &s_sleep_queue) obj->taskctx->last_sleep_tick = get_tick_count();
#endif
    obj->t_next = queue;
    obj->t_prev = queue->t_prev;
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
#ifdef DEBUG
    obj->taskctx->last_wait_tick = get_tick_count();
#endif
    obj->w_next = queue;
    obj->w_prev = queue->w_prev;
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
    KOBJECT *k;
    obj->flags &= ~FFTASK_TASK_TIMEOUT;
    for (k = s_sleep_queue.t_next; k != &s_sleep_queue; k = k->t_next) {
        if ((int32_t)obj->taskctx->timeout - (int32_t)k->taskctx->timeout < 0) break;
    }
    obj->t_next = k;
    obj->t_prev = k->t_prev;
    k->t_prev->t_next = obj;
    k->t_prev = obj;
}

static uint32_t get_tick_count(void) { return *REG_FFVM_MTIMECURL; }
static void* idle_proc(void *arg)    { while (1) {} return NULL;   }

static TASKCTX* task_schedule(KOBJECT *obj, int arg)
{
    uint32_t tick = get_tick_count();
    KOBJECT *prev_task = s_running_task, *k, *t;
    int32_t  next = 0x3fffffff;

#ifdef DEBUG
    s_running_task->taskctx->total_run_tick += tick - s_running_task->taskctx->last_run_tick;
#endif

    if (!obj) {
        for (k = s_sleep_queue.t_next; k != &s_sleep_queue && (int32_t)tick - (int32_t)k->taskctx->timeout >= 0; ) {
            t = k; t->flags |= FFTASK_TASK_TIMEOUT; k = k->t_next;
            w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t);
        }
        s_running_task = s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_TASK) {
        s_running_task = obj->task.joiner ? obj->task.joiner : s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_MUTEX) {
        obj->mutex.owner = obj->w_next != obj ? obj->w_next : NULL;
        s_running_task   = obj->w_next != obj ? obj->w_next : s_ready_queue.t_next;
    } else if (obj->type == FFTASK_KOBJ_COND) {
        s_running_task   = obj->w_next != obj ? obj->w_next : s_ready_queue.t_next;
        if (arg) { // broadcast
            for (k = obj->w_next->w_next; k != obj; ) { t = k; k = k->w_next; w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t); }
        }
    } else if (obj->type == FFTASK_KOBJ_SEM) {
        if (obj->w_next != obj) {
            s_running_task = obj->w_next;
            for (arg--, k = obj->w_next->w_next; k != obj && arg; arg--) { t = k; k = k->w_next; w_dequeue(t); t_dequeue(t); t_enqueue(&s_ready_queue, t); }
        } else {
            s_running_task = s_ready_queue.t_next;
        }
        obj->sem.val += arg;
    }

    if (s_running_task == &s_ready_queue) s_running_task = s_idle_task;
    w_dequeue(s_running_task); t_dequeue(s_running_task);

    if (s_running_task != prev_task) {
        if (prev_task == s_idle_task) { // if prev task is idle task
            *REG_FFVM_CPU_FREQ = 0xFFFFFFFF;  // switch to max cpu freq
            s_stat_idle_total += tick - s_stat_idle_start;
        } else if (s_running_task == s_idle_task) { // if next task is idle task
            *REG_FFVM_CPU_FREQ = 1000 * 1000; // 1MHz cpu freq
            s_stat_idle_start  = tick;
        }
    }

    if (s_ready_queue.t_next != &s_ready_queue) next = 20;
    else if (s_sleep_queue.t_next != &s_sleep_queue) {
        next = (int32_t)s_sleep_queue.t_next->taskctx->timeout - (int32_t)get_tick_count();
    }
    timer_interrupt_next(next);

#ifdef DEBUG
    s_running_task->taskctx->last_run_tick = tick;
#endif
    return s_running_task->taskctx;
}

static void task_entry(KOBJECT *task)
{
    TASKCTX *taskctx = NULL;
    task->taskctx->exitcode = (uint32_t)task->taskctx->taskproc(task->taskctx->taskarg);
    interrupt_off();
    if (task->flags & FFTASK_TASK_DETACH) {
        free(task);
        taskctx = task_schedule(NULL, 0);
    } else {
        task->flags |= FFTASK_TASK_DEAD;
        taskctx = task_schedule(task, 0);
    }
    task_switch_then_interrupt_on(taskctx);
}

static KOBJECT* task_create_internal(char *name, void* (*taskproc)(void*), void *taskarg, int stacksize)
{
    KOBJECT *task = malloc(sizeof(KOBJECT) + sizeof(TASKCTX) + stacksize);
    if (!task) return NULL;
    memset(task, 0, sizeof(KOBJECT) + sizeof(TASKCTX));
    if (name) strncpy(task->name, name, sizeof(task->name) - 1);
    register uint32_t gp asm("gp");
    task->t_next = task->t_prev = task->w_next = task->w_prev = task;
    task->taskctx           = (TASKCTX*)(task + 1);
    task->taskctx->pc       = (uint32_t)task_entry;
    task->taskctx->ra       = (uint32_t)task_entry;
    task->taskctx->a0       = (uint32_t)task;
    task->taskctx->sp       = stacksize ? (uint32_t)((uint8_t*)(task->taskctx + 1) + stacksize) : 0;
    task->taskctx->gp       = gp;
    task->taskctx->taskproc = taskproc;
    task->taskctx->taskarg  = taskarg;
    return task;
}

static KOBJECT* (*s_eintr_handler)(void*) = NULL;
static void      *s_eintr_hdlctxt         = NULL;
TASKCTX* task_eintr_handler(void)
{
    if (!s_eintr_handler) return s_running_task->taskctx;
    KOBJECT *task = s_eintr_handler(s_eintr_hdlctxt);
    if (!task) task = s_running_task;
    else {
        t_enqueue(&s_ready_queue, s_running_task);
        t_dequeue(task); s_running_task = task;
        *REG_FFVM_CPU_FREQ = 0xFFFFFFFF; // switch to max cpu freq
    }
    return task->taskctx;
}

void task_kernel_init(void)
{
    interrupt_off();

    timer_interrupt_init();

    s_ready_queue.t_next = s_ready_queue.t_prev = &s_ready_queue;
    s_sleep_queue.t_next = s_sleep_queue.t_prev = &s_sleep_queue;
    s_objct_queue.t_next = s_objct_queue.t_prev = &s_objct_queue;

    s_idle_task    = task_create_internal("idle", idle_proc, NULL, 256);
    s_main_task    = s_running_task = task_create_internal("main", NULL, NULL, 0);
    s_old_mscratch = csr_read(RISCV_CSR_MSCRATCH);
    csr_write(RISCV_CSR_MSCRATCH, (uint32_t)s_main_task->taskctx);

    s_stat_start_tick = get_tick_count();
    interrupt_on();
}

void task_kernel_exit(void)
{
    KOBJECT *obj, *tmp;
    interrupt_off();
    csr_write(RISCV_CSR_MSCRATCH, s_old_mscratch);
    for (obj = s_ready_queue.t_next; obj != &s_ready_queue; ) { tmp = obj; obj = obj->t_next; t_dequeue(tmp); free(tmp); }
    for (obj = s_sleep_queue.t_next; obj != &s_sleep_queue; ) { tmp = obj; obj = obj->t_next; t_dequeue(tmp); free(tmp); }
    for (obj = s_objct_queue.t_next; obj != &s_objct_queue; ) { tmp = obj; obj = obj->t_next; t_dequeue(tmp); free(tmp); }
    free(s_running_task); s_main_task = s_idle_task = s_running_task = NULL;
    timer_interrupt_exit();
    interrupt_on();
}

void task_kernel_set_eintr_handler(KOBJECT* (*handler)(void*), void *hdlctxt)
{
    s_eintr_handler = handler;
    s_eintr_hdlctxt = hdlctxt;
}

static void kobject_dump(KOBJECT *obj)
{
    if (!obj) return;
    static char *str_tab_type[] = { "task", "mutex", "cond", "sem" };
    printf("kobject: %p, type: %d, %s, name: %s\n", obj, obj->type, str_tab_type[obj->type % 4], obj->name);
    switch (obj->type) {
    case FFTASK_KOBJ_TASK:
        printf("- joiner  : %p\n" , obj->task.joiner);
        printf("- flags   : %lx\n", obj->flags);
        printf("- timeout : %lu\n", obj->taskctx->timeout );
        printf("- exitcode: %lu\n", obj->taskctx->exitcode);
        printf("- taskproc: %p\n" , obj->taskctx->taskproc);
        printf("- taskarg : %p\n" , obj->taskctx->taskarg );
#ifdef DEBUG
        printf("- last_run_tick  : %lu\n", obj->taskctx->last_run_tick  );
        printf("- last_ready_tick: %lu\n", obj->taskctx->last_ready_tick);
        printf("- last_sleep_tick: %lu\n", obj->taskctx->last_sleep_tick);
        printf("- last_wait_tick : %lu\n", obj->taskctx->last_wait_tick );
        printf("- total_run_tick : %lu\n", obj->taskctx->total_run_tick );
#endif
        break;
    case FFTASK_KOBJ_MUTEX:
        printf("- owner: %p\n", obj->mutex.owner);
        break;
    case FFTASK_KOBJ_SEM:
        printf("- value: %lu\n", obj->sem.val);
        break;
    }
    printf("\n");
}

void task_kernel_dump(char *title, char *type, int flag)
{
    if (flag) interrupt_off();
    uint32_t tick  = get_tick_count();
    uint32_t total = tick - s_stat_start_tick + 1;
    KOBJECT *obj   = NULL;

    printf("+--------- %s ---------+\n", title);
    if (strstr(type, "0x") == type) {
        uint32_t addr = 0;
        sscanf(type, "%lx", &addr);
        kobject_dump((KOBJECT*)addr);
        goto done;
    }

    if (strcmp(type, "all") == 0) type = "running + main + idle + stat + ready + sleep + object";

    if (strstr(type, "running")) {
        printf("current tick: %lu\n", tick);
        printf("running task:\n");
        kobject_dump(s_running_task);
    }
    if (strstr(type, "main")) {
        printf("main task:\n");
        kobject_dump(s_main_task);
    }
    if (strstr(type, "idle")) {
        printf("idle task:\n");
        kobject_dump(s_idle_task);
    }

    if (strstr(type, "stat")) {
        printf("s_stat_intr_count: %lu\n", s_stat_intr_count);
        printf("s_stat_start_tick: %lu\n", s_stat_start_tick);
        printf("s_stat_tick_total: %lu\n", total);
        printf("s_stat_idle_start: %lu\n", s_stat_idle_start);
        printf("s_stat_idle_total: %lu\n", s_stat_idle_total);
        printf("s_stat_cpu_usage : %ld\n", 100 * (total - s_stat_idle_total) / total);
        printf("\n");
        s_stat_start_tick = get_tick_count();
        s_stat_idle_total = 0;
    }

    if (strstr(type, "ready" )) {
        printf("ready queue :\n");
        for (obj = s_ready_queue.t_next; obj != &s_ready_queue; obj = obj->t_next) kobject_dump(obj);
        printf("\n");
    }
    if (strstr(type, "sleep" )) {
        printf("sleep queue :\n");
        for (obj = s_sleep_queue.t_next; obj != &s_sleep_queue; obj = obj->t_next) kobject_dump(obj);
        printf("\n");
    }
    if (strstr(type, "object")) {
        printf("object queue:\n");
        for (obj = s_objct_queue.t_next; obj != &s_objct_queue; obj = obj->t_next) kobject_dump(obj);
        printf("\n");
    }

done:
    if (flag) interrupt_on();
}

TASKCTX* task_timer_schedule(void)
{
    s_stat_intr_count++;
    if (s_running_task != s_idle_task) t_enqueue(&s_ready_queue, s_running_task);
    return task_schedule(NULL, 0);
}

KOBJECT* task_create(char *name, void* (*taskfunc)(void*), void *taskarg, int stacksize, int flags)
{
    if (!taskfunc) return NULL;
    interrupt_off();
    KOBJECT *task = task_create_internal(name, taskfunc, taskarg, stacksize ? stacksize : (64 * 1024));
    if (task) {
        if (flags & FFTASK_TASK_CREATE_DETACH) task->flags |= FFTASK_TASK_DETACH;
        t_enqueue(&s_ready_queue, task);
    }
    interrupt_on();
    return task;
}

int task_join(KOBJECT *task, uint32_t *exitcode)
{
    interrupt_off();
    if (!task || task->type != FFTASK_KOBJ_TASK || (task->flags & FFTASK_TASK_DETACH) || task == s_running_task || task->task.joiner) { interrupt_on(); return -1; }
    if (!(task->flags & FFTASK_TASK_DEAD)) {
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
    if (task->flags & FFTASK_TASK_DEAD) free(task);
    else task->flags |= FFTASK_TASK_DETACH;
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

KOBJECT* task_self(void) { return s_running_task; }

KOBJECT* mutex_init(char *name)
{
    KOBJECT *mutex = calloc(1, sizeof(KOBJECT));
    if (!mutex) return NULL;
    if (name) strncpy(mutex->name, name, sizeof(mutex->name) - 1);
    mutex->type = FFTASK_KOBJ_MUTEX;
    mutex->t_next = mutex->t_prev = mutex->w_next = mutex->w_prev = mutex;
    t_enqueue(&s_objct_queue, mutex);
    return mutex;
}

int mutex_destroy(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.owner || mutex->w_next != mutex) { interrupt_on(); return -1; }
    t_dequeue(mutex); free(mutex);
    interrupt_on();
    return 0;
}

int mutex_lock(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (mutex->mutex.owner) {
        w_enqueue(mutex, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
    } else {
        mutex->mutex.owner = s_running_task;
        interrupt_on();
    }
    return 0;
}

int mutex_unlock(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.owner != s_running_task) { interrupt_on(); return -1; }
    if (mutex->w_next != mutex) {
        t_enqueue(&s_ready_queue, s_running_task);
        task_switch_then_interrupt_on(task_schedule(mutex, 0));
    } else {
        mutex->mutex.owner = NULL;
        interrupt_on();
    }
    return 0;
}

int mutex_trylock(KOBJECT *mutex)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX || mutex->mutex.owner) { interrupt_on(); return -1; }
    mutex->mutex.owner = s_running_task;
    interrupt_on();
    return 0;
}

int mutex_timedlock(KOBJECT *mutex, int32_t ms)
{
    interrupt_off();
    if (!mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    if (mutex->mutex.owner) {
        s_running_task->taskctx->timeout = get_tick_count() + ms;
        s_insert(s_running_task); w_enqueue(mutex, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
        return mutex->mutex.owner == s_running_task ? 0 : -1;
    } else {
        mutex->mutex.owner = s_running_task;
        interrupt_on();
        return 0;
    }
}

KOBJECT* cond_init(char *name)
{
    KOBJECT *cond = calloc(1, sizeof(KOBJECT));
    if (!cond) return NULL;
    if (name) strncpy(cond->name, name, sizeof(cond->name) - 1);
    cond->type = FFTASK_KOBJ_COND;
    cond->t_next = cond->t_prev = cond->w_next = cond->w_prev = cond;
    t_enqueue(&s_objct_queue, cond);
    return cond;
}

int cond_destroy(KOBJECT *cond)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || cond->w_next != cond) { interrupt_on(); return -1; }
    t_dequeue(cond); free(cond);
    interrupt_on();
    return 0;
}

int cond_wait(KOBJECT *cond, KOBJECT *mutex)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || !mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    w_enqueue(cond, s_running_task);
    task_switch_then_interrupt_on(task_schedule(mutex, 0));
    return 0;
}

int cond_timedwait(KOBJECT *cond, KOBJECT *mutex, int32_t ms)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || !mutex || mutex->type != FFTASK_KOBJ_MUTEX) { interrupt_on(); return -1; }
    s_running_task->taskctx->timeout = get_tick_count() + ms;
    s_insert(s_running_task); w_enqueue(cond, s_running_task);
    task_switch_then_interrupt_on(task_schedule(mutex, 0));
    return (s_running_task->flags & FFTASK_TASK_TIMEOUT) ? -1 : 0;
}

int cond_signal(KOBJECT *cond, int broadcast)
{
    interrupt_off();
    if (!cond || cond->type != FFTASK_KOBJ_COND || cond->w_next == cond) { interrupt_on(); return -1; }
    t_enqueue(&s_ready_queue, s_running_task);
    task_switch_then_interrupt_on(task_schedule(cond, broadcast));
    return 0;
}

KOBJECT* semaphore_init(char *name, int val)
{
    KOBJECT *sem = calloc(1, sizeof(KOBJECT));
    if (!sem) return NULL;
    if (name) strncpy(sem->name, name, sizeof(sem->name) - 1);
    sem->type    = FFTASK_KOBJ_SEM;
    sem->sem.val = val;
    sem->t_next  = sem->t_prev = sem->w_next = sem->w_prev = sem;
    t_enqueue(&s_objct_queue, sem);
    return sem;
}

int semaphore_destroy(KOBJECT *sem)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_SEM || sem->w_next != sem) { interrupt_on(); return -1; }
    t_dequeue(sem); free(sem);
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
    if (!sem || sem->type != FFTASK_KOBJ_SEM) { interrupt_on(); return -1; }
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
    if (!sem || sem->type != FFTASK_KOBJ_SEM) { interrupt_on(); return -1; }
    if (sem->sem.val == 0) {
        s_running_task->taskctx->timeout = get_tick_count() + ms;
        s_insert(s_running_task); w_enqueue(sem, s_running_task);
        task_switch_then_interrupt_on(task_schedule(NULL, 0));
        return (s_running_task->flags & FFTASK_TASK_TIMEOUT) ? -1 : 0;
    } else {
        sem->sem.val--;
        interrupt_on();
        return 0;
    }
}

int semaphore_post(KOBJECT *sem, int n)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_SEM || n <= 0) { interrupt_on(); return -1; }
    if (sem->w_next != sem) {
        t_enqueue(&s_ready_queue, s_running_task);
        task_switch_then_interrupt_on(task_schedule(sem, n));
    } else {
        sem->sem.val += n;
        interrupt_on();
    }
    return 0;
}

KOBJECT* semaphore_post_isr(KOBJECT *sem)
{
    KOBJECT *task = NULL;
    if (!sem || sem->type != FFTASK_KOBJ_SEM) return task;
    if (sem->w_next != sem) {
        task = sem->w_next;
        w_dequeue(task); t_enqueue(&s_ready_queue, task);
    } else {
        sem->sem.val++;
    }
    return task;
}

int semaphore_getvalue(KOBJECT *sem, int *val)
{
    interrupt_off();
    if (!sem || sem->type != FFTASK_KOBJ_SEM) { interrupt_on(); return -1; }
    if (val) *val = sem->sem.val;
    interrupt_on();
    return 0;
}
