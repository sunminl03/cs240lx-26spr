// this is roughly the gprof example from lab 4
#include "rpi.h"
#include "ss-pixie.h"
static inline void cycle_cnt_init(void) {
    uint32_t in = 1;
    asm volatile("MCR p15, 0, %0, c15, c12, 0" :: "r"(in));
}

void notmain(void) {
    // caches_enable();     // Q: what happens if you enable cache?
    cycle_cnt_init();
    pixie_verbose(0);
    pixie_start();
    for(int i = 0; i < 10; i++) 
        output("%d: hello world\n", i);
    unsigned n = pixie_stop();

    output("done: %d instructions!\n", n);

    // this should dump out the counts.
    pixie_dump(100);
}
