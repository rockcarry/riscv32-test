/* smode_test.c - S-mode test, outputs to UART (0xFF000800) and stdout (0xFF000000) */

#define satp     0x180
#define sstatus  0x100
#define sie      0x104
#define stvec    0x105
#define sscratch 0x140
#define sepc     0x141
#define scause   0x142
#define stval    0x143
#define sip      0x144
#define medeleg  0x302
#define mideleg  0x303
#define mstatus  0x300
#define mie      0x304
#define mepc     0x341
#define mtvec    0x305

static volatile char *uart = (volatile char*)0xFF000800;

/* stubs for libffvm fatfs/fftask references */
void f_mount(void) {}
void mutex_init(void) {}
void mutex_destroy(void) {}
void mutex_timedlock(void) {}
void mutex_unlock(void) {}

static void puts_s(const char *s)
{
    while (*s) {
        *uart = *s++;
        for (volatile int d = 0; d < 10000; d++);
    }
}

static void putx(unsigned v)
{
    static const char hex[] = "0123456789abcdef";
    puts_s("0x");
    for (int i = 28; i >= 0; i -= 4) {
        *uart = hex[(v >> i) & 0xF];
        for (volatile int d = 0; d < 10000; d++);
    }
}

/* S-mode trap handler */
__attribute__((section(".text.strap"), aligned(4)))
void s_trap_handler(void)
{
    unsigned cause, epc;
    asm volatile("csrr %0, scause" : "=r"(cause));
    asm volatile("csrr %0, sepc"   : "=r"(epc));

    if (cause == 9) {
        puts_s("[PASS] ecall from S-mode: scause=9\r\n");
        epc += 4;
    } else {
        puts_s("[FAIL] S-mode trap: scause=");
        putx(cause);
        puts_s("\r\n");
    }
    asm volatile("csrw sepc, %0" :: "r"(epc));
    asm volatile("sret");
}

/* M-mode trap handler */
__attribute__((section(".text.mtrap"), aligned(4)))
void m_trap_handler(void)
{
    unsigned cause, mepc_val;
    asm volatile("csrr %0, mcause" : "=r"(cause));
    asm volatile("csrr %0, mepc"   : "=r"(mepc_val));

    cause &= ~(1u << 31);
    if (cause == 11) {
        puts_s("[PASS] ecall from M-mode: mcause=11\r\n");
        mepc_val += 4;
    } else if (cause == 9) {
        puts_s("[PASS] ecall from S-mode (M trap): mcause=9\r\n");
        { unsigned t = 1 << 9; asm volatile("csrs medeleg, %0" :: "r"(t)); }
        puts_s("       medeleg[9]=1, delegate to S-mode\r\n");
        mepc_val += 4;
    } else {
        puts_s("[FAIL] unexpected mcause=");
        putx(cause);
        puts_s("\r\n");
    }
    asm volatile("csrw mepc, %0" :: "r"(mepc_val));
    asm volatile("mret");
}

int main(void)
{
    puts_s("=== S-mode Test ===\r\n\r\n");

    /* Test 1: M-mode ecall */
    puts_s("Test 1: ecall from M-mode\r\n");
    asm volatile("csrw mtvec, %0" :: "r"((unsigned)m_trap_handler));
    asm volatile("ecall");

    /* Test 2: Enter S-mode, ecall (not delegated yet) */
    puts_s("\r\nTest 2: enter S-mode via mret, ecall\r\n");
    asm volatile("csrw stvec, %0" :: "r"((unsigned)s_trap_handler));

    unsigned m;
    asm volatile("csrr %0, mstatus" : "=r"(m));
    m = (m & ~(3 << 11)) | (1 << 11);  /* MPP = S */
    m |= (1 << 7);                      /* MPIE = 1 */
    m &= ~(1 << 3);                     /* MIE = 0 */
    asm volatile("csrw mstatus, %0" :: "r"(m));

    asm volatile("la t0, 2f; csrw mepc, t0; mret; 2:");

    /* sscratch accessible in S-mode */
    { unsigned s; asm volatile("csrr %0, sscratch" : "=r"(s));
      puts_s("       sscratch read OK\r\n"); }

    asm volatile("ecall");  /* M-mode trap, mcause=9 */

    /* Test 3: ecall delegated to S-mode */
    puts_s("\r\nTest 3: ecall delegated to S-mode\r\n");
    asm volatile("ecall");  /* now delegated, goes to s_trap_handler */

    /* Test 4: sret return */
    puts_s("\r\nTest 4: sret return\r\n");
    asm volatile("ecall");
    puts_s("[PASS] sret returned successfully\r\n");

    puts_s("\r\n=== ALL TESTS DONE ===\r\n");
    while (1);
    return 0;
}
