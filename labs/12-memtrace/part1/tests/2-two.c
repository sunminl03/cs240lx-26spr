#include "rpi.h"
#include "trap-watchpt.h"
 
static void test_interleaved(void) { // two at a time 
    memtrace_trap_disable(); 
    uint32_t *a = kmalloc(4); // two different heap allocated pointers
    uint32_t *b = kmalloc(4);
    memtrace_trap_enable();

    for(int i = 0; i < 10; i++) {
        uint32_t a_val = 0xDEAD0000 + i;
        uint32_t b_val = 0xBEEF0000 + i;
        put32(a, a_val);
        put32(b, b_val);

        uint32_t a_got = get32(a);
        uint32_t b_got = get32(b);

        if(a_got != a_val)
            panic("iter %d a: got=%x expect=%x\n", i, a_got, a_val);
        if(b_got != b_val)
            panic("iter %d b: got=%x expect=%x\n", i, b_got, b_val);
        trace("iter %d: a=%x b=%x ok\n", i, a_got, b_got);
    }
    trace("test_interleaved PASSED\n");
}


void notmain(void) {
    memtrace_init();
    test_interleaved();
    trace("SUCCESS: 2-two.c PASSED\n");
}
