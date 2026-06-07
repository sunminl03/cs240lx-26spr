#include "rpi.h"
#include "rpi-pmu.h"

// 1176, chapter 3, p 3-75:
//   Invalidate Instruction Cache Line using MVA.
//   Prefetch Instruction Cache Line using MVA.
cp_asm(icache_inval_mva, p15, 0, c7, c5, 1)
cp_asm(icache_prefetch_mva, p15, 0, c7, c13, 1)

// instead of invalidating or prefetching the whole icache, we are doing one block of it
// one block or one line of icache is 32bytes long.
static inline void icache_invalidate_mva(const void *addr) {
    icache_inval_mva_set((uint32_t)addr);
}

static inline void icache_prefetch_mva(const void *addr) {
    icache_prefetch_mva_set((uint32_t)addr);
}

enum {
    icache_block = 32, // each line is 32 bytes 
    n_nops = 256,

    arm_nop     = 0xe320f000,
    arm_bx_lr   = 0xe12fff1e,
    arm_mov_r0  = 0xe3a00000,
};

static uint32_t jit_code[258];

static inline uint32_t arm_mov_r0_imm8(uint32_t x) {
    assert(x < 256);
    return arm_mov_r0 | x;
}

static void jit_ret(uint32_t x) {
    for(unsigned i = 0; i < n_nops; i++)
        jit_code[i] = arm_nop;
    jit_code[n_nops] = arm_mov_r0_imm8(x);
    jit_code[n_nops+1] = arm_bx_lr;
}

static uint32_t run_jit(void) {
    uint32_t (*fp)(void) = (void*)jit_code; // treat the address of jit_code as the address of a function.
    return fp();
}

static void for_each_jit_block(void (*fn)(const void *)) {
    char *p = (void*)jit_code; 
    unsigned nbytes = sizeof jit_code;

    for(unsigned off = 0; off < nbytes; off += icache_block)
        fn(p + off); // invalidate or prefetch every single block or line 
}

static void jit_icache_invalidate_all(void) { // this is invalidating every single block
    for_each_jit_block(icache_invalidate_mva);
}

static void jit_icache_prefetch_all(void) {
    for_each_jit_block(icache_prefetch_mva);
}

static uint32_t measure_jit_call(const char *msg, uint32_t expected, uint32_t *n_inst) {
    uint32_t miss, inst = 0;

    pmu_stmt_measure_set(miss, inst,
        msg,
        icache_miss, inst_cnt,
        {
            uint32_t ret = run_jit();
        });

    if(n_inst)
        *n_inst = inst;
    return miss;
}

// testing same thing as 2 but using mva(invalidating only part of icache)
void notmain(void) {
    uint32_t old = 10; // we are going to put in this value and see without invaldiate if this is there
    uint32_t new = 20; 

    pmu_on();
    caches_enable();

    jit_ret(old); // put in code to array 
    jit_icache_invalidate_all(); // invalidate all icache so cpu fetches these code 
    uint32_t r = run_jit(); // pc will now point to jitcode and try to fetch instructions of that address 
    output("initial JIT result=%d\n", r); // we will fetch them from cpu because icache is invalid 
    assert(r == old);

    jit_ret(new);
    uint32_t stale = run_jit(); // cpu wants code in the address of jitcode. address is in icache. still valid. fetch that instead of getting fresh data
    output("after patch without MVA invalidate: got=%d expected stale=%d\n",
        stale, old);
    assert(stale == old);


    icache_invalidate_mva(&jit_code[n_nops]); // only invalidate the part where we move value to $r0. 
    uint32_t good = run_jit(); // they will fetch the new value 
    output("after patch with MVA invalidate: got=%d expected fresh=%d\n",
        good, new);
    assert(good == new);

    uint32_t invalid_inst, prefetched_inst;

    jit_icache_invalidate_all(); // we are going to fetch everything again 
    uint32_t invalid_miss = measure_jit_call("after MVA invalidates: should miss",
        new, &invalid_inst);

    jit_icache_invalidate_all(); // invalidate everything, but then prefetch everything
    jit_icache_prefetch_all();
    uint32_t prefetched_miss = measure_jit_call("after MVA prefetches: fewer misses",
        new, &prefetched_inst);

    output("summary: cold misses=%d inst=%d; prefetched misses=%d inst=%d\n", invalid_miss, invalid_inst, prefetched_miss, prefetched_inst);
}
