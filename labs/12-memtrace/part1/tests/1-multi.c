#include "rpi.h"
#include "trap-watchpt.h"

static void test_multi_allocs(void) {

    memtrace_trap_disable();
    uint32_t *ptrs[6];
    for(int i = 0; i < 6; i++)
        ptrs[i] = kmalloc(4); // 6 different allocs
    memtrace_trap_enable();

    // write to all ptrs in the heap
    for(int i = 0; i < 6; i++) {
        uint32_t val = 0xDEADBEE0 + i;
        put32(ptrs[i], val);
    }

    // read back the values we wrote
    for(int i = 0; i < 6; i++) {
        uint32_t expect = 0xDEADBEE0 + i;
        uint32_t got    = get32(ptrs[i]);
        if(got != expect)
            panic("ptr[%d]: got=%x expect=%x\n", i, got, expect);
        trace("ptr[%d] ok: %x\n", i, got);
    }
    trace("test_separate_allocs PASSED\n");
}

void notmain(void) {
    memtrace_init();

    test_multi_allocs();
    trace("SUCCESS: 1-multi.c PASSED\n");
}
