// THIS CODE is interrupt-driven design.
#include "rpi.h"
#include "rpi-interrupts.h"

// do a raw dump of all the values for a given keypress. 
//
// this is useful for seeing how your remote behaves.
#include "rpi.h"
#include "instr.h"
#include "rpi-inline-asm.h"

// simple-minded log of timed reads (value, usec)
// you should probably write your own.
enum { TR_MAX = 255 };
typedef struct timed_reads {
    unsigned n;
    struct timed_read { 
        uint32_t usec;
        uint32_t v;
    } r[TR_MAX];
} tr_t;

// initialize and return.
static inline tr_t tr_mk(void) {
    return (tr_t){};
}

// return timed read element at <i>: null if none.
struct timed_read *tr_elem(tr_t *t, unsigned i) {
    if(i >= t->n)
        return 0;
    return &t->r[i];
}

// FIFO: add entry to timed read log <l>
static void tr_push(tr_t *l, uint32_t v, uint32_t usec) {
    if(l->n >= TR_MAX)
        panic("too many entries!\n");
    let e = &l->r[l->n++];
    e->usec = usec;
    e->v = v;
}

// loop while(gpio_read(pin) == v) until either:
//   1. the pin changes: return the number of usec passed.
//   2. <timeout> is exceeded: return 0.
static uint32_t read_while_eq(int pin, int v, unsigned timeout) {
    unsigned start = timer_get_usec_raw();
    while(1) {
        // we add +1 to make sure always return != 0
        if(gpio_read(pin) != v)
            return timer_get_usec_raw() - start + 1;
        // if timeout, return 0.
        if((timer_get_usec_raw() - start) >= timeout)
            return 0;
    }
}


char* cmp (char out[100][10]) {
    for (int i = 0; i < 100; i++) {
        if (strcmp(out[i], UP[i])) {
            break;
        } else if (!strcmp(out[i],"STOP")) {
            return "UP";
        }
    } 
    for (int i = 0; i < 100; i++) {
        if (strcmp(out[i], DOWN[i])) {
            break;
        } else if (!strcmp(out[i],"STOP")) {
            return "DOWN";
        }
    }
    for (int i = 0; i < 100; i++) {
        if (strcmp(out[i], RIGHT[i])) {
            break;
        } else if (!strcmp(out[i],"STOP")) {
            return "RIGHT";
        }
    }
    for (int i = 0; i < 100; i++) {
        if (strcmp(out[i], LEFT[i])) {
            break;
        } else if (!strcmp(out[i],"STOP")) {
            return "LEFT";
        }
    }
    for (int i = 0; i < 100; i++) {
        if (strcmp(out[i], STAR[i])) {
            break;
        } else if (!strcmp(out[i],"STOP")) {
            return "STAR";
        }
    }
    for (int i = 0; i < 100; i++) {
        if (strcmp(out[i], HASHTAG[i])) {
            break;
        } else if (!strcmp(out[i],"STOP")) {
            return "HASHTAG";
        }
    }

    return "boo";
}

// similar to our timer interrupt vector but for GPIO.
// from lab 8
void interrupt_vector() {
    dev_barrier();

    if (!gpio_event_detected(21))
        return;
    gpio_event_clear(21);
    // IR START
    enum { 
        pin = 21,         // input pin: "S" on IR
        N = 10,             // total readings
        timeout = 40000,    // timeout in usec
     };

    output("will try to do a raw dump of %d readings\n", N);
    output("  recall readings are \"reversed\":\n");
    output("    no reading ==> gpio_read(pin)=1\n");
    output("    reading    ==> gpio_read(pin)=0\n");

    // save the instructions 
    char instr[100][10] = {};
    uint32_t pin_val, t, idx;

    // output("trial %d: about to read\n", i);
    tr_t l = tr_mk();

    // again: default is 1, so nothing is happening.
    while((pin_val = gpio_read(pin)) == 1)
        ;

    // read values until timeout
    for(idx = 0;  idx < 255; idx++) {
        // read until gpio_read(pin) != v or timeout
        if(!(t = read_while_eq(pin, pin_val, timeout))) {
            // push the timeout too for debugging.
            tr_push(&l, pin_val, timeout);
            break;
        }
        tr_push(&l, pin_val, t);
        // flip so get the next value
        pin_val = 1 - pin_val;
        assert(pin_val == gpio_read(pin));
    }

    // print them out, two at a time so it's easy to see 
    // whats going on.
    for(unsigned i = 0; i < idx; i += 2) {
        let e1 = tr_elem(&l, i);
        assert(e1);
        uint32_t e1_usec = e1->usec;
        output("%d: pin%d=%d: usec=%d ", i, pin, e1->v, e1_usec);
        // in case we don't have enough readings.
        let e2 = tr_elem(&l, i+1);
        uint32_t e2_usec = e2->usec;
        if(e2)
            output("pin%d=%d, usec=%d", pin, e2->v, e2_usec);
        
        if ((e1_usec <= 9900 && e1_usec >= 8100) && (e2_usec >= 4050 && e2_usec <= 4950)) {
            strcpy(instr[i], "HEADER");
        }
        if ((e1_usec >= 500 && e1_usec <=660)) {
            if (e2_usec >= (1600-160) && e2_usec <= (1600+160)) { // 2nd signal is high 
                strcpy(instr[i], "SENT1");
            } else if (e2_usec >= (470) && e2_usec <= (600+60)){ // 2nd signal is low
                strcpy(instr[i], "SENT0");
            } else if (e2_usec >= 600 && e2_usec <= 1600) { // if the 2nd signal is in the middle 
                if (e2_usec <= 1100) {
                    strcpy(instr[i], "SENT0");
                } else {
                    strcpy(instr[i], "SENT1");
                }
            } else if (e2_usec > 3000) {
                strcpy(instr[i], "STOP");
            } else {
                strcpy(instr[i], "IDK");
            }
            
        }

        output("\n");
    }
    char new_instr[100][10] = {};
    int cnt = 0;
    for (int j = 0; j < 100; j++) {
        if (instr[j][0] != '\0') {
            output("%s\n", instr[j]);
            strcpy(new_instr[cnt], instr[j]);
            cnt++;
        }   
    }
    output("-----------------%s---------------------\n", cmp(new_instr));
        
    // if (gpio_event_detected(pin)) {
    //     gpio_event_clear(pin);
    // }
    dev_barrier();
}

void notmain(void) {
    enum { 
        pin = 21,         // input pin: "S" on IR
        N = 10,             // total readings
        timeout = 40000,    // timeout in usec
     };
    // EXTENSION: initialize vector
    interrupt_init();

    gpio_set_input(pin);
    // IR goes to 0 when there is signal.
    // We use a pullup to make sure no signal = 1
    // for sure.
    gpio_set_pullup(pin);     

    // EXTENSION: enable rising edge detection
    gpio_int_rising_edge(pin);
    // gpio_int_falling_edge(pin);

    gpio_event_clear(pin);
    // EXTENSION: enable interrupts
    uint32_t cpsr = cpsr_int_enable();

    // if this fails, your hardware isn't hooked up right
    assert(gpio_read(pin) == 1);

    delay_ms(100);

    while(1) {
        // do nothing, wait for interrupt
    }

    // interrupt_vector();
}
