#include "rpi.h"
#include "rpi-pmu.h"

// 1176, chapter 3, p 3-22: cache type reg
cp_asm_get(cache_type, p15, 0, c0, c0, 1)

// Reuse the cache operations from the previous tests so the PMU experiment can
// make a invalid/valid comparison over generated code.
cp_asm(icache_reg, p15, 0, c7, c5, 0)
cp_asm(icache_inval_mva, p15, 0, c7, c5, 1)

static inline void icache_invalidate(void) {
    icache_reg_set(0);
}

static inline void icache_invalidate_mva(const void *addr) {
    icache_inval_mva_set((uint32_t)addr);
}

typedef struct {
    unsigned present;
    unsigned size_field;
    unsigned assoc_field;
    unsigned m;
    unsigned len_field;
    unsigned line_bytes;
    unsigned ways;
    unsigned size_bytes;
    unsigned sets;
} cache_info_t;

// helper so that we can get specific bits from it 
static unsigned bits(uint32_t x, unsigned lo, unsigned hi) {
    unsigned nbits = hi - lo + 1;
    return (x >> lo) & ((1 << nbits) - 1);
}

// from page 3-22. 
static cache_info_t decode_cache(uint32_t r, unsigned is_dcache) {
    cache_info_t c;

    if(is_dcache) {
        c.size_field = bits(r, 18, 21);
        c.assoc_field = bits(r, 15, 17);
        c.m = bits(r, 14, 14);
        c.len_field = bits(r, 12, 13);
    } else {
        c.size_field = bits(r, 6, 9);
        c.assoc_field = bits(r, 3, 5);
        c.m = bits(r, 2, 2);
        c.len_field = bits(r, 0, 1);
    }

    c.present = !c.m;
    c.line_bytes = (1 << (c.len_field + 1)) * 4;
    c.ways = 1 << c.assoc_field;
    c.size_bytes = 512 << c.size_field;
    c.sets = c.present ? c.size_bytes / (c.line_bytes * c.ways) : 0;

    return c;
}

static void print_cache_info(const char *name, cache_info_t c) {
    if(!c.present) {
        output("%s-cache: absent\n", name);
        return;
    }

    output("%s-cache: size=%d bytes, line=%d bytes, assoc=%d-way, sets=%d\n",
        name, c.size_bytes, c.line_bytes, c.ways, c.sets);
    output("  raw fields: size=%d assoc=%d M=%d len=%d\n",
        c.size_field, c.assoc_field, c.m, c.len_field);
}

enum {
    max_test_lines = 64,
    arm_nop = 0xe320f000,
    arm_mov_r0_7 = 0xe3a00007,
    arm_bx_lr = 0xe12fff1e,
};

static uint32_t jit_code[max_test_lines * 8 + 2] __attribute__((aligned(32)));

static void jit_make_n_lines(unsigned line_bytes, unsigned nlines) {
    // fill in jit_code as big as icache 
    unsigned nwords = (line_bytes / 4) * nlines;

    for(unsigned i = 0; i < nwords; i++)
        jit_code[i] = arm_nop;
    jit_code[nwords] = arm_mov_r0_7;
    jit_code[nwords+1] = arm_bx_lr;
    dmb();
    gcc_mb();
}

static uint32_t run_jit(void) { // pc will jump to address of jit code
    uint32_t (*fp)(void) = (void*)jit_code;
    return fp();
}

static void invalidate_jit_by_mva(unsigned line_bytes, unsigned nlines) {
    char *p = (void*)jit_code;
    unsigned nbytes = line_bytes * nlines;
    // invalidate part of icache 
    for(unsigned off = 0; off < nbytes; off += line_bytes)
        icache_invalidate_mva(p + off);
}

static uint32_t measure_jit_call(const char *msg, uint32_t *n_inst) {
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

void notmain(void) {

    uint32_t r = cache_type_get(); // gives us the full data
    unsigned ctype = bits(r, 25, 28); // read cache type 

    output("cache type register raw=0x%x\n", r);
    output("ctype=%d\n", ctype);

    cache_info_t d = decode_cache(r, 1); // all information stored in this cache 
    cache_info_t i = decode_cache(r, 0);
    print_cache_info("D", d);
    print_cache_info("I", i);

    pmu_on();
    caches_enable();
    assert(caches_is_enabled());

    unsigned test_lines = i.sets < max_test_lines ? i.sets : max_test_lines;
    // unsigned test_lines = i.sets;
    if(test_lines == 0)
        test_lines = 1;

    jit_make_n_lines(i.line_bytes, test_lines);  // number of bytes per line, and number of lines
    // filling in all the icache 


    icache_invalidate();
    uint32_t invalid_inst, valid_inst, mva_inst;
    uint32_t invalid_miss = measure_jit_call("JIT call after whole i-cache invalidate", &invalid_inst); // this should be a lot of misses cuz cache is invalidated
    uint32_t valid_miss = measure_jit_call("good JIT call: claimed lines should hit", &valid_inst); // this is not because w ealready loaded it in before

    // invalidate the icache lines that stored all this
    invalidate_jit_by_mva(i.line_bytes, test_lines);
    uint32_t mva_miss = measure_jit_call("after invalidating claimed MVA lines", &mva_inst);

    output("claimed line bytes=%d, tested lines=%d\n", i.line_bytes, test_lines);
    output("summary: invalid misses=%d inst=%d; valid misses=%d inst=%d; mva-cold misses=%d inst=%d\n", invalid_miss, invalid_inst, valid_miss, valid_inst, mva_miss, mva_inst);
}
