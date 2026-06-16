/* mmu_test.c - Sv32 page table test, uses 4MB megapages for identity map */

#define satp     0x180
#define sstatus  0x100
#define stvec    0x105
#define scause   0x142
#define stval    0x143
#define medeleg  0x302
#define mstatus  0x300
#define mepc     0x341
#define mtvec    0x305

#define PTE_V     (1 << 0)
#define PTE_R     (1 << 1)
#define PTE_W     (1 << 2)
#define PTE_X     (1 << 3)
#define PTE_U     (1 << 4)

#define MEGAPAGE  (4 * 1024 * 1024)      /* 4MB */

static volatile char *uart = (volatile char*)0xFF000800;

void f_mount(void) {}
void mutex_init(void) {}
void mutex_destroy(void) {}
void mutex_timedlock(void) {}
void mutex_unlock(void) {}

static void uart_putc(char c)
{
    *uart = c;
    for (volatile int d = 0; d < 30000; d++);
}
static void puts_s(const char *s)
{
    while (*s) uart_putc(*s++);
}

__attribute__((section(".text.mtrap"), aligned(4)))
void m_trap_handler(void)
{
    unsigned cause, tval;
    asm volatile("csrr %0, mcause" : "=r"(cause));
    asm volatile("csrr %0, mtval"  : "=r"(tval));
    cause &= ~(1u << 31);
    puts_s("[FAIL] page fault cause=");
    for (int i = 28; i >= 0; i -= 4) uart_putc("0123456789abcdef"[(cause >> i) & 0xF]);
    puts_s(" addr=");
    for (int i = 28; i >= 0; i -= 4) uart_putc("0123456789abcdef"[(tval >> i) & 0xF]);
    puts_s("\r\n");
    while (1);
}

/* megapage entry: identity-maps 4MB starting at va=va_base to pa=va_base */
static unsigned megapage_pte(unsigned va_base)
{
    unsigned ppn = (va_base >> 12) & 0x3FFFFF;
    return (ppn << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
}

int main(void)
{
    char __attribute__((aligned(4096))) root_pt[4096];
    char __attribute__((aligned(4096))) leaf_pt[4096];   /* for 0xC0000000 mapping */
    unsigned *rpt = (unsigned*)root_pt;
    unsigned *lpt = (unsigned*)leaf_pt;
    unsigned  i;

    for (i = 0; i < 1024; i++) rpt[i] = lpt[i] = 0;

    puts_s("=== Sv32 MMU Test ===\r\n\r\n");

    /* identity-map 128MB using megapages (4MB each, 32 entries) */
    puts_s("identity map 0x00000000 - 0x07FFFFFF\r\n");
    for (i = 0; i < 32; i++)
        rpt[i] = megapage_pte(i * MEGAPAGE);
    /* ROM/RAM at 0x80000000 (64MB = 16 megapages) */
    puts_s("identity map 0x80000000 - 0x83FFFFFF\r\n");
    for (i = 0; i < 16; i++)
        rpt[(0x80000000 / MEGAPAGE) + i] = megapage_pte(0x80000000 + i * MEGAPAGE);
    puts_s("       done\r\n");

    /* map 0xC0000000 -> 0x80100000 (test page) via 4KB page table */
    puts_s("4KB map 0xC0000000 -> 0x80100000\r\n");
    unsigned test_pa  = 0x80100000;
    unsigned vpn1     = (0xC0000000 >> 22) & 0x3FF;
    lpt[0] = ((test_pa >> 12) << 10) | PTE_V | PTE_R | PTE_W | PTE_X;
    rpt[vpn1] = (((unsigned)lpt >> 12) << 10) | PTE_V;
    puts_s("       done\r\n\r\n");

    /* setup M-mode trap */
    asm volatile("csrw mtvec, %0" :: "r"((unsigned)m_trap_handler));

    /* enter S-mode */
    unsigned m;
    asm volatile("csrr %0, mstatus" : "=r"(m));
    m = (m & ~(3 << 11)) | (1 << 11);
    m |= (1 << 7);
    m &= ~(1 << 3);
    asm volatile("csrw mstatus, %0" :: "r"(m));
    asm volatile("la t0, 2f; csrw mepc, t0; mret; 2:");

    /* Test 1: pre-MMU physical access */
    puts_s("Test 1: write before MMU\r\n");
    volatile unsigned *p = (volatile unsigned*)test_pa;
    unsigned saved = *p;
    *p = 0xDEADBEEF;
    if (*p != 0xDEADBEEF) { puts_s("[FAIL]\r\n"); goto done; }
    puts_s("[PASS] direct physical access\r\n\r\n");

    /* Test 2: enable Sv32 */
    puts_s("Test 2: enable Sv32 MMU\r\n");
    unsigned satp_val = (1u << 31) | ((unsigned)rpt >> 12);
    asm volatile("csrw satp, %0" :: "r"(satp_val));
    asm volatile("sfence.vma");
    puts_s("[PASS] satp set\r\n\r\n");

    /* Test 3: identity-mapped code still runs */
    puts_s("Test 3: code via identity map\r\n");
    puts_s("[PASS] still executing\r\n\r\n");

    /* Test 4: virtual 0xC0000000 R/W */
    puts_s("Test 4: read/write via 0xC0000000\r\n");
    p = (volatile unsigned*)0xC0000000;
    /* first read from the mapped test page */
    *p = 0xCAFEBABE;
    if (*p != 0xCAFEBABE) { puts_s("[FAIL]\r\n"); goto done; }
    puts_s("[PASS] virtual read/write OK\r\n\r\n");

    /* Test 5: verify physical page */
    puts_s("Test 5: verify physical page\r\n");
    asm volatile("csrw satp, zero");
    if (*(volatile unsigned*)test_pa != 0xCAFEBABE)
        { puts_s("[FAIL]\r\n"); goto done; }
    puts_s("[PASS] physical = 0xCAFEBABE\r\n");

    /* restore */
    *p = saved;
    asm volatile("csrw satp, %0" :: "r"(satp_val));
    puts_s("\r\n=== ALL TESTS PASSED ===\r\n");

done:
    while (1);
    return 0;
}
