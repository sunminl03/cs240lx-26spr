#include "rpi.h"

#define NELEM(x) (sizeof(x) / sizeof((x)[0]))
#include "cycle-util.h"

typedef void (*int_fp)(void);

static volatile unsigned cnt = 0;

// fake little "interrupt" handlers: useful just for measurement.
void int_0() { cnt++; }
void int_1() { cnt++; }
void int_2() { cnt++; }
void int_3() { cnt++; }
void int_4() { cnt++; }
void int_5() { cnt++; }
void int_6() { cnt++; }
void int_7() { cnt++; }

void generic_call_int(int_fp *intv, unsigned n) { 
    for(unsigned i = 0; i < n; i++)
        intv[i]();
}
// Added since we need to push lr before calling bl 
static inline uint32_t armv6_push(int reg) {
    assert(reg<16);
    // todo("return the machine code to push{reg}\n");
    return 0b11101001001011010000000000000000 | (1 << reg);
}

static inline uint32_t armv6_pop(int reg) {
    assert(reg<16);
    // todo("return the machine code to pop{reg}\n");
    return 0b11101000101111010000000000000000 | (1 << reg);
}

static inline uint32_t armv6_bl(uint32_t bl_pc, uint32_t target) {
    // todo("return the machine code bl to <addr>\n");
    int32_t offset = (int32_t) (target - (bl_pc + 8)) >> 2; // make it int32_t to preserve sign 
    return (0b11101011 << 24) | (offset & 0x00FFFFFF);
}

static inline uint32_t armv6_b(uint32_t bl_pc, uint32_t target) {
    // todo("return the machine code bl to <addr>\n");
    int32_t offset = (int32_t) (target - (bl_pc + 8)) >> 2; // make it int32_t to preserve sign 
    return (0b11101010 << 24) | (offset & 0x00FFFFFF);
}

static inline uint32_t armv6_bx(uint32_t reg) {
    assert(reg<16);
    return (0b1110000100101111111111110001) << 4 | reg;
    // todo("return the machine code to bx <reg>\n");
}

void jit_int(void *fn) {
    // a few of the registers
    enum {
        lr = 14,
        pc = 15,
        sp = 13,
        r0 = 0,
    };
    uint32_t addr =(uint32_t)fn;
    static uint32_t code[5];
    uint32_t n = 0;
    // 1. save lr
    code[n++] = armv6_push(lr);
    uint32_t src = (uint32_t)&code[n]; // the instruction address of armv6_push(lr)
    // 2. call bl. branch to int(1)
    code[n++] = armv6_bl(src, addr);
    code[n++] = armv6_pop(lr);
    code[n++] = armv6_bx(lr);

    void (*fp)(void) = (typeof(fp))code;
    fp();
}

// you will generate this dynamically.
// specialized_call_int is a function that takes (int_fp *intv, unsigned n) 
// and returns a function pointer to a function of type void f(void).
static void (*specialized_call_int(int_fp *intv, unsigned n))(void) {
    
    // a few of the registers
    enum {
        lr = 14,
        pc = 15,
        sp = 13,
        r0 = 0,
    };
    static uint32_t code[100];
    uint32_t j = 0;

    // save the return address (Eventually where we need to return) in code[0]
    code[j++] = armv6_push(lr);
    
    // for the first 7 int calls, we call bl. we set the return address so that we can come back here.
    for (int i = 0; i < (n-1); i++) {
        uint32_t src = (uint32_t)&code[j];
        code[j++] = armv6_bl(src, (uint32_t)intv[i]);
    }

    // we need our original lr in place 
    code[j++] = armv6_pop(lr);
    // call last branch 
    uint32_t src = (uint32_t)&code[j];
    code[j++] = armv6_b(src, (uint32_t)intv[n-1]);

    // void (*fp)(void) = (typeof(fp))code;
    // fp();
    return (void (*)(void))code;
}

void notmain(void) {
    int_fp intv[] = {
        int_0,
        int_1,
        int_2,
        int_3,
        int_4,
        int_5,
        int_6,
        int_7
    };

    cycle_cnt_init();

    unsigned n = NELEM(intv);

    // try with and without cache: but if you modify the routines to do 
    // jump-threadig, must either:
    //  1. generate code when cache is off.
    //  2. invalidate cache before use.
    // enable_cache();

    cnt = 0;
    TIME_CYC_PRINT10("cost of generic-int calling",  generic_call_int(intv,n));
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    // rewrite to generate specialized caller dynamically.
    cnt = 0;
    // specialized_call_int will build the code once. return a pointer to the trampoline 
    // this is less costly than building code every single time we run the code (10 times)
    void (*trampoline)(void) = specialized_call_int(intv, n);
    // call the generated code. 
    TIME_CYC_PRINT10("cost of specialized int calling", trampoline());
    demand(cnt == n*10, "cnt=%d, expected=%d\n", cnt, n*10);

    clean_reboot();
}
