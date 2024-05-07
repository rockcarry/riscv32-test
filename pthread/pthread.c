#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
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
