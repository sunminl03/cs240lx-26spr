#include "rpi.h"
#include "measure.h"

void notmain(void) {
    asm_pmu_t n8, n32; // store results here
    asm_measure_8_nops(&n8); // call routines from assembly
    asm_measure_32_nops(&n32);

    output("8 nops:  inst=%d branch=%d cycles=%d\n", n8.inst, n8.branch, n8.cycles);
    output("32 nops: inst=%d branch=%d cycles=%d\n", n32.inst, n32.branch, n32.cycles);
}
