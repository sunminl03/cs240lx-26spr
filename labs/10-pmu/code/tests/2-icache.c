#include "rpi.h"
#include "rpi-pmu.h"

// 1176, chapter 3, p 3-74: Invalidate Entire Instruction Cache.
// This also flushes the branch target cache / BTAC.
cp_asm(icache_reg, p15, 0, c7, c5, 0)

static inline void icache_invalidate(void) {
    icache_reg_set(0);
}

enum {
    n_nops = 256,

    arm_nop     = 0xe320f000, // got these from .list
    arm_bx_lr   = 0xe12fff1e,
    arm_mov_r0  = 0xe3a00000, // to move it to $r0, we need to OR it 
};

static uint32_t jit_code[258];

static inline uint32_t arm_mov_r0_imm8(uint32_t x) {
    return arm_mov_r0 | x; 
}

static void jit_ret(uint32_t x) {
    for(unsigned i = 0; i < n_nops; i++)
        jit_code[i] = arm_nop;
    jit_code[n_nops] = arm_mov_r0_imm8(x);
    jit_code[n_nops+1] = arm_bx_lr;
}

static uint32_t run_jit(void) {
    uint32_t (*fp)(void) = (void*)jit_code; // take addr of code array and branch to it; 
    return fp(); // at this point, CPU treats the code as ARM instructions
}

// count icache misses and instruction cnts. return icache misses
static uint32_t measure_jit_call(const char *msg, uint32_t expected, uint32_t *n_inst) {
    uint32_t miss, inst, ret = 0;

    pmu_stmt_measure_set(miss, inst,
        msg,
        icache_miss, inst_cnt,
        {
            ret = run_jit();
        });

    assert(ret == expected);
    if(n_inst)
        *n_inst = inst;
    return miss;
}

// testing if icache works by using pmu. 
// if icache is filled with valid entries, should have less cycles
// if icache is invalidated, should have more cycles
void notmain(void) {
    uint32_t old = 10; // we are going to put in this value and see without invaldiate if this is there
    uint32_t new = 20; 

    pmu_on();
    caches_enable();
    assert(caches_is_enabled());

    
    jit_ret(old); // have "code" store instrucitons with old val 
    icache_invalidate(); // get rid of all past instructions 
    uint32_t r = run_jit(); // CPU pc moves to the address of jit_code, CPU starts fetching instructions from these array 
    // CPU wants instructions at address jitcode. but sometimes icache has it, sometimes doesn't
    output("initial JIT result=%d\n", r);
    assert(r == old);

    jit_ret(new); // put code into array ** it is written to the same address old instructions were written to
    uint32_t r2 = run_jit(); // cache is still valid, so the cache does not know if we need to fetch from ram 
    output("after patch without invalidate: got=%d expected stale=%d\n", r2, old);
    assert(r2 == old);

    icache_invalidate(); // now invalidate cache 
    uint32_t shouldbenew = run_jit(); // cpu has to now refetch from RAM
    output("after patch with invalidate: got=%d expected shouldbenew=%d\n", shouldbenew, new);
    assert(shouldbenew == new);

    assert(run_jit() == new);
    // at this point, all new code is in cache 
    uint32_t icache_valid_inst, icache_invalid_inst; // save the instruction count 
    uint32_t icache_valid_miss = measure_jit_call("instructions already stored and icache valid: should mostly hit", new, &icache_valid_inst);
    icache_invalidate();
    // run the whole thing again, but the cache is invalid so cpu has to fetch it again from RAM
    // will be lots of misses 
    uint32_t icache_invalid_miss = measure_jit_call("after invalidate: should miss", new, &icache_invalid_inst);

    output("summary: warm misses=%d inst=%d; after-inval misses=%d inst=%d\n", icache_valid_miss, icache_valid_inst, icache_invalid_miss, icache_invalid_inst);
}
