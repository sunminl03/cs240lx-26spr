// part 1: uses your GPIO code to blink a single LED connected to 
// pin 20.
//   - when run should keep blinking.
//   - to restart: you have to power-cycle the pi.
#include "rpi.h"
#include "gpio_raw.h"
#include "gpio.h"
#include "ss-pixie.h"
static inline void cycle_cnt_init(void) {
    uint32_t in = 1;
    asm volatile("MCR p15, 0, %0, c15, c12, 0" :: "r"(in));
}

void notmain(void) {
    enum { led = 27 };
    cycle_cnt_init();
    pixie_verbose(0);
    pixie_start();

    gpio_set_output(led);
    for (int i = 0; i < 10; i++) {
        gpio_set_on_raw(led);
        delay_cycles(100);
        gpio_set_off_raw(led);
        delay_cycles(100);
    }
    unsigned n = pixie_stop();

    output("done: %d instructions!\n", n);

    // this should dump out the counts.
    pixie_dump(1);
}
