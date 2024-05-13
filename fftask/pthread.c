#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#define _POSIX_TIMERS
#define _POSIX_MONOTONIC_CLOCK
#include <time.h>
#include "fftask.h"
#include "pthread.h"

void libpthread_init(void)
{
    task_kernel_init();
}

void libpthread_exit(void)
{
    task_kernel_exit();
}

int pthread_create(pthread_t *thread, pthread_attr_t *attr, void* (*start_routine)(void*), void *arg)
{
    if (!thread) return -1;
    int stacksize = attr ? attr->stacksize   : 0;
    int flags     = attr ? attr->detachstate : 0;
    *thread = task_create(NULL, start_routine, arg, stacksize, flags);
    return *thread ? 0 : -1;
}

int pthread_join(pthread_t thread, void **retval)
{
    uint32_t exitcode = 0;
    int ret = task_join(thread, &exitcode);
    if (retval) *retval = (void*)exitcode;
    return ret;
}

int pthread_detach(pthread_t thread)
{
    return task_detach(thread);
}

pthread_t pthread_self(void) { return task_self(); }

int pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t *attr)
{
    if (!mutex) return -1;
    KOBJECT *obj = mutex_init(NULL);
    if (obj && attr && *attr == CLOCK_MONOTONIC) obj->flags |= FFTASK_MONOTONIC_TIME;
    *mutex = obj;
    return obj ? 0 : -1;
}

int pthread_mutex_lock(pthread_mutex_t *mutex)
{
    if (!mutex) return -1;
    return mutex_unlock(*mutex);
}

int pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec *ts)
{
    if (!mutex || !ts) return -1;
    KOBJECT *obj = *mutex;
    if (!obj) return -1;
    struct timespec now;
    clock_gettime((obj->flags & FFTASK_MONOTONIC_TIME) ? CLOCK_MONOTONIC : CLOCK_REALTIME, &now);
    int32_t ms = (ts->tv_sec * 1000 + ts->tv_nsec / 1000000) - (now.tv_sec * 1000 + now.tv_nsec / 1000000);
    return mutex_timedlock(obj, ms);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex)
{
    if (!mutex) return -1;
    return mutex_trylock(*mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex)
{
    if (!mutex) return -1;
    return mutex_unlock(*mutex);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex)
{
    if (!mutex) return -1;
    return mutex_destroy(*mutex);
}

int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *attr)
{
    if (!cond) return -1;
    KOBJECT *obj = cond_init(NULL);
    if (obj && attr && *attr == CLOCK_MONOTONIC) obj->flags |= FFTASK_MONOTONIC_TIME;
    *cond = obj;
    return obj ? 0 : -1;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if (!cond || !mutex) return -1;
    return cond_wait(*cond, *mutex);
}

int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, struct timespec *ts)
{
    if (!cond || !mutex || !ts) return -1;
    KOBJECT *obj = *cond;
    if (!obj) return -1;

    struct timespec now;
    clock_gettime((obj->flags & FFTASK_MONOTONIC_TIME) ? CLOCK_MONOTONIC : CLOCK_REALTIME, &now);
    int32_t ms = (ts->tv_sec * 1000 + ts->tv_nsec / 1000000) - (now.tv_sec * 1000 + now.tv_nsec / 1000000);
    return cond_timedwait(*cond, *mutex, ms);
}

int pthread_cond_signal(pthread_cond_t *cond)
{
    if (!cond) return -1;
    return cond_signal(*cond, 0);
}

int pthread_cond_broadcast(pthread_cond_t *cond)
{
    if (!cond) return -1;
    return cond_signal(*cond, 1);
}

int pthread_cond_destroy(pthread_cond_t *cond)
{
    if (!cond) return -1;
    return cond_destroy(*cond);
}

int sem_init(sem_t *sem, int pshared, int value)
{
    if (!sem) return -1;
    *sem = semaphore_init(NULL, value);
    return sem ? 0 : -1;
}

int sem_destroy(sem_t *sem)
{
    if (!sem) return -1;
    return semaphore_destroy(*sem);
}

int sem_trywait(sem_t *sem)
{
    if (!sem) return -1;
    return semaphore_trywait(*sem);
}

int sem_wait(sem_t *sem)
{
    if (!sem) return -1;
    return semaphore_wait(*sem);
}

int sem_timedwait(sem_t *sem, struct timespec *ts)
{
    if (!sem || !ts) return -1;
    KOBJECT *obj = *sem;
    if (!obj) return -1;

    struct timespec now;
    clock_gettime((obj->flags & FFTASK_MONOTONIC_TIME) ? CLOCK_MONOTONIC : CLOCK_REALTIME, &now);
    int32_t ms = (ts->tv_sec * 1000 + ts->tv_nsec / 1000000) - (now.tv_sec * 1000 + now.tv_nsec / 1000000);
    return semaphore_timedwait(obj, ms);
}

int sem_post(sem_t *sem)
{
    if (!sem) return -1;
    return semaphore_post(*sem, 1);
}

int sem_post_multiple(sem_t *sem, int count)
{
    if (!sem) return -1;
    return semaphore_post(*sem, count);
}

int sem_getvalue(sem_t *sem, int *val)
{
    if (!sem) return -1;
    return semaphore_getvalue(*sem, val);
}

int pthread_spin_init(pthread_spinlock_t *l, int pshared)
{
    if (!l) return -1;
    atomic_int *atom = malloc(sizeof(atomic_int));
    if (!atom) return -1;
    atomic_init(atom, 0);
    *l = (pthread_spinlock_t)atom;
    return 0;
}

int pthread_spin_destroy(pthread_spinlock_t *l)
{
    if (!l) return -1;
    free(*l);
    return 0;
}

int pthread_spin_lock(pthread_spinlock_t *l)
{
    if (!l || !*l) return -1;
    atomic_int *atom = (atomic_int*)*l;
    while (atomic_exchange(atom, 1)) {
        while (atomic_load(atom));
    }
    return 0;
}

int pthread_spin_trylock(pthread_spinlock_t *l)
{
    if (!l || !*l) return -1;
    atomic_int *atom = (atomic_int*)*l;
    return atomic_exchange(atom, 1) == 0 ? 0 : -1;
}

int pthread_spin_unlock(pthread_spinlock_t *l)
{
    if (!l || !*l) return -1;
    atomic_int *atom = (atomic_int*)*l;
    atomic_store(atom, 0);
    return 0;
}
