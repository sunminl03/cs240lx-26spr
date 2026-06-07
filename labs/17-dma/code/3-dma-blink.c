// Same as 1-dma-blink, except now the `delay_ms` is also DMA
#include "rpi.h"
#include "dma-impl.h"
#include "gpio-raw.h"
#include "pi-random.h"
// enum { GPIO_BASE  = 0x20200000 };
enum { timeout = 10000000 };
void gpio_set_on_dma(dma_ch_t *dma, unsigned pin) {
    bus_t set0_bus_addr = arm_to_bus((uint32_t)GPIO_SET0);
    // static: DMA reads this asynchronously after dma_initiate returns
    static uint32_t pin_val;
    pin_val = 0x1u << pin;
    cb_t cb = cb_mk(set0_bus_addr, bus(&pin_val), 4);
    dma_run(dma, &cb, 1024);
}

void gpio_set_off_dma(dma_ch_t *dma, unsigned pin) {
    bus_t clr0_bus_addr = arm_to_bus((uint32_t)GPIO_CLR0);
    static uint32_t pin_val;
    pin_val = 0x1u << pin;
    cb_t cb = cb_mk(clr0_bus_addr, bus(&pin_val), 4);
    dma_run(dma, &cb, 1024);
}
#define MHz 700UL

// convert nanoseconds to pi cycles: you need to modify this if you change
// the pi's clock speed.
#define ns_to_cycles(x) (unsigned) (((unsigned)x * 7UL) / 10UL )

void dma_delay_ms(dma_ch_t *dma, unsigned ms) {
    // SRC_INC reads through different cache lines (like part 2).
    // WAITS_MAX adds 28 - 33 so about 30 dummy cycles per bus transaction, slowing it further.
    enum { CHUNK = 10000 }; // 10000 bytes 
    static uint32_t src[CHUNK/4];
    static volatile uint32_t dst;

    // Starting estimate from part 2: (ms-2)*100 bytes. Recalibrate from output.
    uint32_t total = (ms <= 2) ? 4 : (ms - 2) * 6000;

    uint32_t remaining = total;
    while (remaining > 0) { //  loop reuses the same 10,000-byte buffer repeatedly
        uint32_t n;
        if (remaining > CHUNK) {
            n = CHUNK;
        } else {
            n = remaining;
        }
        cb_t cb = {};
        cb.TI = SRC_INC | (WAITS_MAX << WAITS_OFFSET);
        cb.SRC_ADDR = bus(&src[0]);
        cb.DST_ADDR = bus(&dst);
        cb.TXFR_LEN = n;
        // uint32_t t0 = cycle_cnt_read();
        if (!dma_run(dma, &cb, 100*1000*1000))
            panic("timed out\n");
        // uint32_t t1 = cycle_cnt_read();
        // output("cycles=%d for %d bytes\n", t1-t0, n);
        remaining -= n;
    }
}
// 700 million cycles per second 
void notmain(void) {
    // parthiv board led
    enum { pin = 27 };
    gpio_set_output(pin);

    enum { N = 40 };

#if 0
    // if you want to test with normal GPIO to make sure
    // works.
    output("going to blink parthiv board\n");
    for(int i = 0; i < N; i++) {
        gpio_set_on(pin);
        delay_ms(1000);
        gpio_set_off(pin);
        delay_ms(1000);
    }
#endif

    enum { dma_ch = 4 };
    let dma = dma_init(dma_ch);

    output("going to blink with DMA\n");
    for(int i = 0; i < 4; i++) {
        gpio_set_on_dma(dma, pin);
        dma_delay_ms(dma, 3000);
        gpio_set_off_dma(dma, pin);
        dma_delay_ms(dma, 3000);
    }
}
