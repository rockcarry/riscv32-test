#ifndef __SYS_ARCH_H__
#define __SYS_ARCH_H__

typedef void* sys_mutex_t;
typedef void* sys_sem_t;

#ifndef MAX_MBOX_SIZE
#define MAX_MBOX_SIZE 100
#endif
typedef struct {
    int size, head, tail;
    void *lock;
    void *cond;
    void *data[MAX_MBOX_SIZE];
} sys_mbox_t;

typedef void* sys_thread_t;
typedef void* sys_prot_t;

#endif
