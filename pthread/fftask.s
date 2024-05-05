.macro save_context base
    sw ra , 0  (\base)
    sw sp , 4  (\base)
    sw gp , 8  (\base)
    sw tp , 12 (\base)
    sw t0 , 16 (\base)
    sw t1 , 20 (\base)
    sw t2 , 24 (\base)
    sw s0 , 28 (\base)
    sw s1 , 32 (\base)
    sw a0 , 36 (\base)
    sw a1 , 40 (\base)
    sw a2 , 44 (\base)
    sw a3 , 48 (\base)
    sw a4 , 52 (\base)
    sw a5 , 56 (\base)
    sw a6 , 60 (\base)
    sw a7 , 64 (\base)
    sw s2 , 68 (\base)
    sw s3 , 72 (\base)
    sw s4 , 76 (\base)
    sw s5 , 80 (\base)
    sw s6 , 84 (\base)
    sw s7 , 88 (\base)
    sw s8 , 92 (\base)
    sw s9 , 96 (\base)
    sw s10, 100(\base)
    sw s11, 104(\base)
    sw t3 , 108(\base)
    sw t4 , 112(\base)
    sw t5 , 116(\base)
#   sw t6 , 120(\base) # t6 used for context base addr
.endm

.macro load_context base
    lw ra , 0  (\base)
    lw sp , 4  (\base)
    lw gp , 8  (\base)
    lw tp , 12 (\base)
    lw t0 , 16 (\base)
    lw t1 , 20 (\base)
    lw t2 , 24 (\base)
    lw s0 , 28 (\base)
    lw s1 , 32 (\base)
#   lw a0 , 36 (\base) # a0 used for context base addr
    lw a1 , 40 (\base)
    lw a2 , 44 (\base)
    lw a3 , 48 (\base)
    lw a4 , 52 (\base)
    lw a5 , 56 (\base)
    lw a6 , 60 (\base)
    lw a7 , 64 (\base)
    lw s2 , 68 (\base)
    lw s3 , 72 (\base)
    lw s4 , 76 (\base)
    lw s5 , 80 (\base)
    lw s6 , 84 (\base)
    lw s7 , 88 (\base)
    lw s8 , 92 (\base)
    lw s9 , 96 (\base)
    lw s10, 100(\base)
    lw s11, 104(\base)
    lw t3 , 108(\base)
    lw t4 , 112(\base)
    lw t5 , 116(\base)
    lw t6 , 120(\base)
.endm

.global task_switch
.align 4
task_switch:
    csrrw t6, mscratch, t6 # swap t6 <-> mscratch
    beqz t6, _load1
    save_context t6
    mv t5, t6
    csrr t6, mscratch
    sw t6, 120(t5)
    lw t6, 0(t5)
    sw t6, 124(t5)

_load1:
    csrw mscratch, a0
    load_context a0
    lw a0, 36(a0)
    ret

.extern task_schedule
.global task_timer_isr
.align 4
task_timer_isr:
    csrrw t6, mscratch, t6 # t6 <-> mscratch, get context pointer from mscratch
    bnez t6, _save         # if t6 != 0 goto _save
    add t6, sp, -128       # allocate task context on stack

_save:
    save_context t6        # save all regs except t6 & pc
    mv t5, t6              # use t5 as task context pointer
    csrrw t6, mscratch, t6 # t6 <-> mscratch
    sw t6, 120(t5)         # save t6 to task->t6
    csrr t6, mepc          # t6 <- mepc
    sw t6, 124(t5)         # save mepc to task->pc

    jal task_schedule      # do task schedule

    bnez a0, _load2        # if scheduler result not null next task is new task
    csrrw a0, mscratch, a0 # else next task is current task

_load2:
    lw t0, 124(a0)         # t0 <- task->pc
    csrw mepc, t0          # mepc <- t0

    csrw mscratch, a0      # set mscratch to a0 which pointer to new task context
    bne a0, sp, _load3     # if next task is new task
    csrw mscratch, zero    # else set mscratch to zer0

_load3:
    load_context a0        # load all regs except a0
    lw a0, 36(a0)          # load a0 from task->a0
    mret
