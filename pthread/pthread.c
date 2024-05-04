#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include "ffvmreg.h"
#include "libffvm.h"
#include "pthread.h"

static int s_counter;

static void __attribute__((interrupt)) __attribute__((aligned(4))) libpthread_timer_isr(void)
{
    uint64_t mtimecmp = ((*REG_FFVM_MTIMECMPL << 0) | ((uint64_t)*REG_FFVM_MTIMECMPH << 32)) + 20;
    *REG_FFVM_MTIMECMPL = mtimecmp >>  0;
    *REG_FFVM_MTIMECMPH = mtimecmp >> 32;
    s_counter++;
}

static uint32_t read_csr(uint32_t addr)
{
    uint32_t value;
    asm volatile(
        "csrr %0, %1" : "=r"(value) : "i"(addr)
    );
    return value;
}

static void write_csr(uint32_t reg, uint32_t value) {
    asm volatile (
        "csrw %0, %1" :: "i" (reg), "r" (value)
    );
}

void libpthread_init(void)
{
    uint64_t mtimecur = ((*REG_FFVM_MTIMECURL << 0) | ((uint64_t)*REG_FFVM_MTIMECURH << 32)) + 20;
    *REG_FFVM_MTIMECMPL = mtimecur >>  0;
    *REG_FFVM_MTIMECMPH = mtimecur >> 32;

    write_csr(RISCV_CSR_MTVEC, (uint32_t)libpthread_timer_isr); // setup timer isr

    uint32_t val;
    val  = read_csr(RISCV_CSR_MSTATUS);
    val |= (1 << 3); // enable mstatus:mie
    write_csr(RISCV_CSR_MSTATUS, val);

    val  = read_csr(RISCV_CSR_MIE);
    val |= (1 << 7); // enable mie timer interrupt
    write_csr(RISCV_CSR_MIE, val);
}

void libpthread_exit(void)
{
    uint32_t val;
    val  = read_csr(RISCV_CSR_MIE);
    val &=~(1 << 7); // disable mie timer interrupt
    write_csr(RISCV_CSR_MIE, val);
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

int main(void)
{
    libpthread_init();
    while (1) {
        printf("s_counter: %d\n", s_counter);
        mdelay(1000);
    }
    libpthread_exit();
    return 0;
}
