#include "rpi.h"

// See broadcomm documents for magic addresses and magic values.
//
// If you pass addresses as:
//  - pointers use put32/get32.
//  - integers: use PUT32/GET32.
//  semantics are the same.
enum {
    // Max gpio pin number.
    GPIO_MAX_PIN = 53,

    GPIO_BASE = 0x20200000,
    gpio_set0  = (GPIO_BASE + 0x1C),
    gpio_clr0  = (GPIO_BASE + 0x28),
    gpio_lev0  = (GPIO_BASE + 0x34),
    gpio_pud = (GPIO_BASE + 0x94),
    gpio_pudclk0 = (GPIO_BASE + 0x98),
    gpio_pudclk1 = (GPIO_BASE + 0x9C)

    // <you will need other values from BCM2835!>
};

// Set GPIO <pin> = on.
void gpio_set_on_raw(unsigned pin) {
    if(pin > GPIO_MAX_PIN)
        gpio_panic("illegal pin=%d\n", pin);

    // Implement this. 
    // NOTE: 
    //  - If you want to be slick, you can exploit the fact that 
    //    SET0/SET1 are contiguous in memory.

    // 00008024 <PUT32>:
    //  8024:   e5801000    str r1, [r0]
    //  8028:   e12fff1e    bx  lr
    unsigned address1 = 0x2020001C; // GPSET0
    unsigned address2 = 0x20200020; // GPSET1
    unsigned bits = 0x1u << (pin % 32);
    if (pin < 32) {
        asm volatile("str %0, [%1]" :: "r"(bits), "r"(address1) : "memory");
    } else {
        asm volatile("str %0, [%1]" :: "r"(bits), "r"(address2) : "memory");
    }


}

// Set GPIO <pin> = off
void gpio_set_off_raw(unsigned pin) {
    if(pin > GPIO_MAX_PIN)
        gpio_panic("illegal pin=%d\n", pin);

    // Implement this. 
    // NOTE: 
    //  - If you want to be slick, you can exploit the fact that 
    //    CLR0/CLR1 are contiguous in memory.
    unsigned address1 = 0x20200028; // GPCLR0
    unsigned address2 = 0x2020002C; // GPCLR1
    unsigned bits = 0x1u << (pin % 32);
    if (pin < 32){
        asm volatile("str %0, [%1]" :: "r"(bits), "r"(address1) : "memory");
    } else {
        asm volatile("str %0, [%1]" :: "r"(bits), "r"(address2) : "memory");
    }
}