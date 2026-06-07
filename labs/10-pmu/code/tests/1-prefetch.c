#include "rpi.h"
#include "rpi-pmu.h"

// find the size of prefetch buffer by seeing if there is a huge jump 
// in cycle count and instruction stalls when executing n instructions vs n+1 instruction
// wait this might be invalid.

// prefetch buffer: grabbing stuff in chunks 
// ARM has 16 or 32 bytes large prefetch buffer
// lets say we grabbed 16 bytes. but our code is 16 bytes long and starts at offset 4. then we have to fetch multiple times
// so alignment tells us where the first instruction starts inside the fetch block 
// how many instructions fit before the CPU needs another fetch 
__attribute__((noinline)) 
void
measure_nops(const char *msg, int n) {
    // uint32_t cyc_s, cyc_e;
    // uint32_t inst0_s, inst0_e;
    // uint32_t stall1_s, stall1_e;
    asm volatile(".align 5");
    pmu_stmt_measure(msg, 
            inst_cnt, 
            inst_stall, 
    {
        // when align = 5: 9->10, align = 2: 2->3 (instruction stall increases), 
        // when align = 2, it just means start on a normal instruction boundary
        // with this version, the cpu can fetch 2 instructions effectively and 
        // align place code in different places 
        // so if we have 3 nops and not 2 nops, then our real code is starting at offset 12 and might be split badly among chunks 
        asm volatile("nop");  // 1
        asm volatile("nop");  // 2
        asm volatile("nop");  // 3 --> here is when we get instr jump from when align = 2
        // asm volatile("nop");  // 4
        // asm volatile("nop");  // 5

        // asm volatile("nop");  // 6
        // asm volatile("nop");  // 7
        // asm volatile("nop");  // 8
        // asm volatile("nop");  // 9
        // asm volatile("nop");  // 10 --> here is when we get cycle jump from 57-> 81 when align = 5
    });
}

void notmain(void) {
    output("For each alignment, see how prefetch buffer works.\n");

    for(int i = 0; i < 3; i++)
        measure_nops("cache",i);
}