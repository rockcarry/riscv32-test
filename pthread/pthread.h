#ifndef _PTHREAD_H_
#define _PTHREAD_H_

#include <time.h>

typedef void *pthread_t;
typedef void *pthread_mutex_t;
typedef void *pthread_cond_t;
typedef void *pthread_spinlock_t;
typedef void *sem_t;

typedef unsigned pthread_mutexattr_t;
typedef unsigned pthread_condattr_t;

typedef struct {
    int detachstate;
    int stacksize;
} pthread_attr_t;

int pthread_create(pthread_t *thread, pthread_attr_t *attr, void* (*start_routine)(void*), void *arg);
int pthread_join(pthread_t thread, void **retval);
int pthread_detach(pthread_t thread);
pthread_t pthread_self(void);

int pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t *attr);
int pthread_mutex_lock(pthread_mutex_t *mutex);
int pthread_mutex_timedlock(pthread_mutex_t *mutex, struct timespec *ts);
int pthread_mutex_trylock(pthread_mutex_t *mutex);
int pthread_mutex_unlock(pthread_mutex_t *mutex);
int pthread_mutex_destroy(pthread_mutex_t *mutex);

int pthread_cond_init(pthread_cond_t *cond, pthread_condattr_t *attr);
int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex);
int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex, struct timespec *ts);
int pthread_cond_signal(pthread_cond_t *cond);
int pthread_cond_broadcast(pthread_cond_t *cond);
int pthread_cond_destroy(pthread_cond_t *cond);

int pthread_spin_init(pthread_spinlock_t *l, int pshared);
int pthread_spin_destroy(pthread_spinlock_t *l);
int pthread_spin_lock(pthread_spinlock_t *l);
int pthread_spin_trylock(pthread_spinlock_t *l);
int pthread_spin_unlock(pthread_spinlock_t *l);

int sem_init(sem_t *sem, int pshared, int value);
int sem_destroy(sem_t *sem);
int sem_trywait(sem_t *sem);
int sem_wait(sem_t *sem);
int sem_timedwait(sem_t *sem, struct timespec *ts);
int sem_post(sem_t *sem);
int sem_post_multiple(sem_t *sem, int count);
int sem_getvalue(sem_t *sem, int *val);

void libpthread_init(void);
void libpthread_exit(void);

#endif
