// use DMA to blink pin 27 on parthiv board.
//
// NOTE: 
//  - you'll have to implement <arm_to_bus>
//  - probably should use your <gpio-raw.h>
#include "rpi.h"
#include "dma-impl.h"
#include "gpio-raw.h"

// enum { GPIO_BASE  = 0x20200000};
// #define GPIO_CLR0 (void*)(GPIO_BASE + 0x28)
// #define GPIO_SET0 (void*)(GPIO_BASE + 0x1C)

void gpio_set_on_dma(dma_ch_t *dma, unsigned pin) {
    // todo("do a dma write to SET0\n");
    bus_t set0_bus_addr = arm_to_bus((uint32_t)GPIO_SET0); // map to bus addr? 
    uint32_t pin_val = 0x1u << (pin);
    cb_t cb = cb_mk(set0_bus_addr, bus(&pin_val), 4);
    dma_initiate(dma, &cb);
    enum { timeout = 1024 };
    // if(!dma_wait(dma, timeout))
    //     panic("dma timed out!\n");
    // if(dst != src || dst != val)
    //     panic("did not copy?  dst=%x, src=%x expected=%x\n",
    //         dst, src, val);
}

void gpio_set_off_dma(dma_ch_t *dma, unsigned pin) {
    // todo("do a dma write to CLR0\n");
    bus_t clr0_bus_addr = arm_to_bus((uint32_t)GPIO_CLR0); // map to bus addr? 
    uint32_t pin_val = 0x1u << (pin);
    cb_t cb = cb_mk(clr0_bus_addr, bus(&pin_val), 4);
    dma_initiate(dma, &cb);
    enum { timeout = 1024 };
    // if(!dma_wait(dma, timeout))
    //     panic("dma timed out!\n");
    // if(dst != src || dst != val)
    //     panic("did not copy?  dst=%x, src=%x expected=%x\n",
    //         dst, src, val);
}

void notmain(void) {
    // parthiv board led
    enum { pin = 27 };
    gpio_set_output(pin);

    enum { N = 4 };

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
    for(int i = 0; i < N; i++) {
        gpio_set_on_dma(dma, pin);
        delay_ms(1000);
        gpio_set_off_dma(dma, pin);
        delay_ms(1000);
    }
}
