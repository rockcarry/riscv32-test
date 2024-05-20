.macro save_context base
    sw ra , 4  (\base)
    sw sp , 8  (\base)
    sw gp , 12 (\base)
    sw tp , 16 (\base)
#   sw t0 , 20 (\base) # t0 used for context base addr
    sw t1 , 24 (\base)
    sw t2 , 28 (\base)
    sw s0 , 32 (\base)
    sw s1 , 36 (\base)
    sw a0 , 40 (\base)
    sw a1 , 44 (\base)
    sw a2 , 48 (\base)
    sw a3 , 52 (\base)
    sw a4 , 56 (\base)
    sw a5 , 60 (\base)
    sw a6 , 64 (\base)
    sw a7 , 68 (\base)
    sw s2 , 72 (\base)
    sw s3 , 76 (\base)
    sw s4 , 80 (\base)
    sw s5 , 84 (\base)
    sw s6 , 88 (\base)
    sw s7 , 92 (\base)
    sw s8 , 96 (\base)
    sw s9 , 100(\base)
    sw s10, 104(\base)
    sw s11, 108(\base)
    sw t3 , 112(\base)
    sw t4 , 116(\base)
    sw t5 , 120(\base)
    sw t6 , 124(\base)
.endm

.macro load_context base
    lw ra , 4  (\base)
    lw sp , 8  (\base)
    lw gp , 12 (\base)
    lw tp , 16 (\base)
    lw t0 , 20 (\base)
    lw t1 , 24 (\base)
    lw t2 , 28 (\base)
    lw s0 , 32 (\base)
    lw s1 , 36 (\base)
#   lw a0 , 40 (\base) # a0 used for context base addr
    lw a1 , 44 (\base)
    lw a2 , 48 (\base)
    lw a3 , 52 (\base)
    lw a4 , 56 (\base)
    lw a5 , 60 (\base)
    lw a6 , 64 (\base)
    lw a7 , 68 (\base)
    lw s2 , 72 (\base)
    lw s3 , 76 (\base)
    lw s4 , 80 (\base)
    lw s5 , 84 (\base)
    lw s6 , 88 (\base)
    lw s7 , 92 (\base)
    lw s8 , 96 (\base)
    lw s9 , 100(\base)
    lw s10, 104(\base)
    lw s11, 108(\base)
    lw t3 , 112(\base)
    lw t4 , 116(\base)
    lw t5 , 120(\base)
    lw t6 , 124(\base)
.endm

.global task_switch_then_interrupt_on
.align 4
task_switch_then_interrupt_on:
    csrrw t0, mscratch, t0  # t0 <-> mscratch
    save_context t0         # save all regs to task_old except t0
    csrrw t1, mscratch, a0  # t1 <- mscratch <- a0
    sw t1, 20(t0)           # save t0 -> task_old->t0 (note: t1 is actual the orignal t0 value)
    sw ra, 0 (t0)           # save ra -> task_old->pc

    lw t2, 0 (a0)           # t2 <- task_new->pc
    csrw mepc, t2           # mepc <- t2
    load_context a0         # load all regs from task_new except a0
    lw a0, 40(a0)           # load a0 from task_new->a0
    mret

.extern task_timer_schedule
.align 4
task_timer_isr:
    csrrw t0, mscratch, t0  # t0 <-> mscratch
    save_context t0         # save all regs to task_old except t0
    csrrw t1, mscratch, a0  # t1 <- mscratch <- a0
    sw t1, 20(t0)           # save t0 -> task_old->t0 (note: t1 is actual the orignal t0 value)
    csrr ra, mepc           # ra <- mepc
    sw ra, 0 (t0)           # save ra -> task_old->pc

    jal task_timer_schedule # do task schedule, after calling a0 is the return of task_timer_schedule
    csrw mscratch, a0       # mscratch <- a0

    lw t2, 0 (a0)           # t2 <- task_new->pc
    csrw mepc, t2           # mepc <- t2
    load_context a0         # load all regs from task_new except a0
    lw a0, 40(a0)           # load a0 from task_new->a0
    mret

.align 4
task_eintr_isr:
    csrrw t0, mscratch, t0  # t0 <-> mscratch
    save_context t0         # save all regs to task_old except t0
    csrrw t1, mscratch, a0  # t1 <- mscratch <- a0
    sw t1, 20(t0)           # save t0 -> task_old->t0 (note: t1 is actual the orignal t0 value)
    csrr ra, mepc           # ra <- mepc
    sw ra, 0 (t0)           # save ra -> task_old->pc

    jal task_eintr_handler  # external interrupt handler, after calling a0 is the return of task_eintr_handler
    csrw mscratch, a0       # mscratch <- a0

    lw t2, 0 (a0)           # t2 <- task_new->pc
    csrw mepc, t2           # mepc <- t2
    load_context a0         # load all regs from task_new except a0
    lw a0, 40(a0)           # load a0 from task_new->a0
    mret

.extern task_isr_external
.global task_isr_vector
.option norvc
.align 4
task_isr_vector:
    mret                # 0
    mret                # 1
    mret                # 2
    mret                # 3 software interrupt
    mret                # 4
    mret                # 5
    mret                # 6
    j task_timer_isr    # 7 timer interrupt
    mret                # 8
    mret                # 9
    mret                # 10
    j task_eintr_isr    # 11 external interrupt
    mret                # 12
    mret                # 13
    mret                # 14
    mret                # 15
