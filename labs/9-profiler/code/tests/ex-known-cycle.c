#include "rpi.h"
#include "cycle-count.h"

#define NOP10() asm volatile( \
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n" \
    "nop\n" "nop\n" "nop\n" "nop\n" "nop\n" )

void test_cycle_counter(void) {
    uint32_t s, e;

    s = cycle_cnt_read();
    NOP10();
    e = cycle_cnt_read();
    printk("10 nops: %d cycles\n", e - s);

    s = cycle_cnt_read();
    NOP10(); NOP10();
    e = cycle_cnt_read();
    printk("20 nops: %d cycles\n", e - s);

    s = cycle_cnt_read();
    NOP10(); NOP10(); NOP10(); NOP10(); NOP10();
    e = cycle_cnt_read();
    printk("50 nops: %d cycles\n", e - s);
}

void notmain() {
    test_cycle_counter();
    return;
}