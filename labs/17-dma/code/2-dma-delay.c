// Goal: make a DMA chain that takes a certain amount of time to execute.
//
// We'll want this for part 3, because we'll be replacing the `delay_ms`
// function with another DMA operation.

#include "rpi.h"
#include "dma-impl.h"
#include "cycle-count.h"
#include "pi-random.h"
// The amount of time it takes a DMA transfer to run is roughly correlated
// with the number of bytes transferred.
//
// Goal: figure out what that relationship is, and figure out a way
// to get a DMA chain that waits a certain amount of time (e.g. N milliseconds)
//
// you can do this however you want.  Probably what
// you should do is look at the TI register (p50) and
// play around with the different fields to see how
// to (1) slow down a transfer and (2) reduce memory
// traffic.
// 
// Useful: 
//   - delay: WAITS bits(21:25)
//   - wide bursts: bit(26)
//   - ignore src or dst
//   - don't increment source (bit(8)
//   - don't increment dst (bit(4))
//    - wait for response (bit(3))
//  - some others: i didn't mess around too much.
//
// Also useful: 
//   - trim out some of the fat for initiation and wait.
//
// NOTE: 
//   1. you probably don't want to increment source and dst.
//   2. to reduce bus traffic it's good to either ignore
//      dst or src (but not both)
//   3. I couldn't get delays to add enough so I copied bytes
//      too.
//

// r/pi A+ default CPU cycles per sec: 700MHz (700 million cycles per second)
// if you overclock need to change this.
#define MHz 700UL

// convert nanoseconds to pi cycles: you need to modify this if you change
// the pi's clock speed.
#define ns_to_cycles(x) (unsigned) (((unsigned)x * 7UL) / 10UL )

#define cycles_to_ns(x) (unsigned) ((10UL* (unsigned)x) / 7UL)

enum { timeout = 8000 };
static uint32_t measure_block_exec_time(dma_ch_t *dma, uint32_t byte_count) {
    // Generate a DMA chain that copies `byte_count` bytes; use the tricks
    // noted above to figure out how to get a DMA chain that takes the
    // appropriate amount of time.
    //
    // This function should return the number of cycles it takes the chain to
    // run.
    // todo("implement\n");
    int index = byte_count / 4;
    volatile uint32_t src[index], dst[index]; 
    uint32_t exp[index];
    for (int i = 0; i < index; i++) {
        exp[i] = src[i] = pi_random();
    }
    // output("exp = %x, src = %x\n", exp, src);
    cb_t cb = cb_mk(bus(&dst[0]), bus(&src[0]), byte_count);
    uint32_t cycle_start = cycle_cnt_read();
    if(!dma_run(dma, &cb, timeout))
        panic("timed out\n");
    uint32_t cycle_end = cycle_cnt_read();
    for (int i = 0; i < index; i++) {
        if(dst[i] != src[i] || dst[i] != exp[i])
            panic("did not copy?  dst[%d]=%x, src[%d]=%x expected[%d]=%x\n", i, i, i, dst, src, exp);
    }
    
    return cycle_end - cycle_start;

}

void notmain(void) {
    enum { dma_ch = 4 };
    let dma = dma_init(dma_ch);

    cycle_cnt_init();

    // First: establish a baseline measurement, because DMA startup/teardown
    // also has time costs.
    // 
    // baseline was around ~1500 cycles 40 bytes, 1803, 136 bytes 2669, 228 byte 3483, 324 bytes 4324, 412 5121

    // warmup
    output("first run = %d\n", measure_block_exec_time(dma, 4));
    output("second run = %d\n", measure_block_exec_time(dma, 4));
    let baseline = measure_block_exec_time(dma, 4);
    output("baseline=%d\n", baseline);

    // measure for different inputs to `byte_count`

    for(unsigned i = 1, n = 4; n < 512; n += 4, i++) {
        output("running DMA with nbytes = %d\n", n);
        let c = measure_block_exec_time(dma,n);
        output("copied %d bytes took %d cycles, took %d ns, incremental=%d\n", 
            n, 
            c, 
            cycles_to_ns(c),
            (c-baseline)/i);
    }
}
