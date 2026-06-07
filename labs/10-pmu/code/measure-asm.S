@ use this file to put your assembly code: gives more 
@ precise control.
#include "rpi-asm.h"

// prefetch buffer appears to be 32 bytes (2^5)
#define ALIGN .align 5

ALIGN
.globl nop_1
nop_1:
    nop  @ 1
    nop  @ 2
    nop  @ 3
    nop  @ 4
    nop  @ 5

    nop  @ 1
    nop  @ 2
    nop  @ 3
    nop  @ 4
    nop  @ 5

    bx lr

ALIGN
.globl nop_2
nop_2:
    nop  @ 1
    nop  @ 2
    nop  @ 3
    nop  @ 4
    nop  @ 5

    nop  @ 1
    nop  @ 2
    nop  @ 3
    nop  @ 4
    nop  @ 5

    bx lr

@ PMU helpers .
@ Event0 = instruction count (0x7), event1 = branch executed (0x5).
@ value that we are going to put into control register 
#define PMU_CTRL_INST_BRANCH 0x00705703
@ writing into control register to count instruction, branch
#define PMU_CONFIG_INST_BRANCH(tmp) \
    ldr tmp, =PMU_CTRL_INST_BRANCH; \
    mcr p15, 0, tmp, c15, c12, 0


#define PMU_READ_EVENT0(dst) mrc p15, 0, dst, c15, c12, 2
#define PMU_READ_EVENT1(dst) mrc p15, 0, dst, c15, c12, 3
#define PMU_READ_CYCLE(dst)  mrc p15, 0, dst, c15, c12, 1


ALIGN
.globl asm_measure_8_nops
asm_measure_8_nops:
    push {r4-r7, lr}     @ save callee saved regs
    mov r4, r0           @ r4 now stores the address where we can save result
    PMU_CONFIG_INST_BRANCH(r1)
    PMU_READ_EVENT0(r5)  @ start instruction count
    PMU_READ_EVENT1(r6)
    PMU_READ_CYCLE(r7)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    PMU_READ_EVENT0(r0)
    PMU_READ_EVENT1(r1)
    PMU_READ_CYCLE(r2)
    sub r0, r0, r5
    sub r1, r1, r6
    sub r2, r2, r7
    stmia r4, {r0-r2}    @ save result
    pop {r4-r7, pc}


ALIGN
.globl asm_measure_32_nops
asm_measure_32_nops:
    push {r4-r7, lr}
    mov r4, r0
    PMU_CONFIG_INST_BRANCH(r1)
    PMU_READ_EVENT0(r5)
    PMU_READ_EVENT1(r6)
    PMU_READ_CYCLE(r7)
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    nop
    PMU_READ_EVENT0(r0)
    PMU_READ_EVENT1(r1)
    PMU_READ_CYCLE(r2)
    sub r0, r0, r5
    sub r1, r1, r6
    sub r2, r2, r7
    stmia r4, {r0-r2}
    pop {r4-r7, pc}
