#ifndef __MEASURE_H__
#define __MEASURE_H__

#include "rpi.h"

void nop_1();
void nop_2();

typedef struct {
    uint32_t inst;
    uint32_t branch;
    uint32_t cycles;
} asm_pmu_t;

void asm_measure_8_nops(asm_pmu_t *out);
void asm_measure_32_nops(asm_pmu_t *out);

#endif
