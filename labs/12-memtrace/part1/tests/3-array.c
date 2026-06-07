#include "rpi.h"
#include "trap-watchpt.h"

static void test_array_fill(void) {
    memtrace_trap_disable();
    uint32_t *arr = kmalloc(10 * sizeof(uint32_t));
    memtrace_trap_enable();

    for(int i = 0; i < 10; i++)
        put32(arr + i, 0xDEADBEE0 + i);

    for(int i = 0; i < 10; i++) {
        uint32_t expect = 0xDEADBEE0 + i;
        uint32_t got    = get32(arr + i);
        if(got != expect)
            panic("arr[%d]: got=%x expect=%x\n", i, got, expect);
        trace("arr[%d]=%x ok\n", i, got);
    }
    trace("test_array_fill PASSED\n");
}

void notmain(void) {
    memtrace_init();
    test_array_fill();
    trace("SUCCESS: 3-array.c PASSED\n");
}
