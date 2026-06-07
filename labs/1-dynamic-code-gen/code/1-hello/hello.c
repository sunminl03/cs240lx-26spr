// dynamically generate code to call a routine.  we
// do this in two steps to make it easier to debug.
//  - first, generate a call to a predefined functions 
//    that take no arguments (hello_before and hello_after).
//  - when that works, generate a call to a routine that takes
//    a 32-bit argument.  the general way that arm handles this:
//       - write the 32-bit value in the code array
//         *after* all the code you generate
//       - emit an ldr instruction using the pc to load
//         it.
#include "rpi.h"
#include "rpi-interrupts.h"

// we use this to catch if you jump off by one
static void guard1(void) { asm volatile ("bkpt"); }

static void hello1(void) { 
    printk("hello world 1\n");
}

// we use this to catch if you jump off by one
static void guard2(void) { asm volatile ("bkpt"); }

void hello2(void) { 
    printk("hello world 2\n");
}

// we use this to catch if you jump off by one
static void guard3(void) { asm volatile ("bkpt"); }

// the routines to implement.
static inline uint32_t armv6_push(int reg) {
    assert(reg<16);
    // todo("return the machine code to push{reg}\n");
    return 0b11101001001011010000000000000000 | (1 << reg);
}
static inline uint32_t armv6_pop(int reg) {
    assert(reg<16);
    // todo("return the machine code to pop{reg}\n");
    return 0b11101000101111010000000000000000 | (1 << reg);
}

// pc = where the instruction will be put.  this is 
// needed so that you can compute the offset from <pc>
// to <addr> which is what gets put in <bl>
static inline uint32_t armv6_bl(uint32_t bl_pc, uint32_t target) {
    // todo("return the machine code bl to <addr>\n");
    int32_t offset = (int32_t) (target - (bl_pc + 8)) >> 2; // make it int32_t to preserve sign 
    return (0b11101011 << 24) | (offset & 0x00FFFFFF);
}
static inline uint32_t armv6_bx(uint32_t reg) {
    assert(reg<16);
    return (0b1110000100101111111111110001) << 4 | reg;
    // todo("return the machine code to bx <reg>\n");
}

static inline uint32_t armv6_mov(uint32_t arg, uint32_t reg) {
    assert(reg < 16);
    assert(arg < 256); 
    return ((0b11100011101000000000000000000000) | (reg << 12)) | arg;
}

static inline uint32_t 
armv6_ldr(uint32_t dst_reg, uint32_t src_reg, uint32_t off) {
    assert(dst_reg<16);
    assert(src_reg<16);
    // todo("return the machine code to: ldr <dst>, [<src>+#<off>]\n");
    uint32_t base = 0b111001011001;
    base = (base << 20) | (src_reg << 16) | (dst_reg << 12) | off;
    return base;
}

// generate a dynamic call to hello() 
void jit_hello(void *fn, void * arg) {
    static uint32_t code[8];
    

    // a few of the registers
    enum {
        lr = 14,
        pc = 15,
        sp = 13,
        r0 = 0,
    };

    uint32_t addr =(uint32_t)fn;
    uint32_t n = 0;

    // generate a trampoline to call <fn>.
    //   1. we need to save and restore <lr> since the 
    //      call (bl) will trash it.
    //   2. need to make sure you sign extend <addr> in bl
    //      correctly so that it works with a negative offset!
    code[n++] = armv6_push(lr);

    // to see what value the pc register has when you read it
    // you can look at <prelab-code-pi/4-derive-pc-reg.c>
    // or also read the manual :)
    // uint32_t ldr_pc = 0;
    if(arg) {
        // todo("extend this code to handle a 32-bit argument!\n");
        code[n++] = armv6_ldr(r0, pc, 8);                // read the code element that is storing argument. 
        // code[n++] = armv6_mov(r0, (uint32_t)arg);
    }

    // push lr
    // mov r0, #0x12345678; ldr r0, [pc, #8]
    // bl fn
    // pop lr
    // bx lr
    // 0x12345678
    //

    uint32_t src = (uint32_t)&code[n]; // the instruction address of armv6_push(lr)
    code[n++] = armv6_bl(src, addr);
    code[n++] = armv6_pop(lr);
    code[n++] = armv6_bx(lr);
    // write 32-bit argument at the end of code array 
    if (arg) {
        code[n++] = (uint32_t)arg;
    }

    printk("emitted code at %x to call routine (%x):\n", code, addr);
    for(int i = 0; i < n; i++) 
        printk("code[%d]=0x%x\n", i, code[i]);

    void (*fp)(void) = (typeof(fp))code;
    printk("about to call: %x\n", fp);
    printk("--------------------------------------\n");
    fp();
    printk("--------------------------------------\n");
}

void notmain(void) {
    // so we can catch some exceptions.
    interrupt_init();

    // step 1: generate calls that take no arguments.
    jit_hello(hello1, 0);
    jit_hello(hello2, 0);
    // step 2: generate calls that take a single argument
    jit_hello(printk, "hello world\n");
    jit_hello(printk, "hello world2\n");
}
