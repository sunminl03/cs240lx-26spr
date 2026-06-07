#include "rpi.h"
#include "trap-watchpt.h"


void notmain(void) { 
    memtrace_init();
    // make sure trapping is off while we mess with the
    // heap.
    memtrace_trap_disable();

    uint32_t *v = kmalloc(88);

    memtrace_trap_enable(); // remove permissions for that domain. trap start. 

    // do <N> trials where we read/write <v>
    // using GET32/PUT32 and validate the result.
    enum { N = 10 };
    for(int i = 0; i < N; i++) {
        trace("about to do a PUT32!\n");

        // write a 32 bit value so we can
        // make sure no byte got messed up or
        // ignored.
        uint32_t expect = 0xfaf0faf0+i;
        put32(v, expect);
        trace("about to do a GET!\n");
        uint32_t got = get32(v);

        // check that what we read equals what we
        // wrote.
        if(expect != got)
            panic("failed: got=%x, expect=%x\n", got, expect);
        else
            trace("%d: success: got=%x, expect=%x\n", 
                                        i,got, expect);
        output("v = %x", v);
        v += 4;
    }
    trace("SUCCESS!  passsed %d trials\n", N);
}