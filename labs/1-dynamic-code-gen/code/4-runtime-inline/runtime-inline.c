// test framework to check and time runtime inlining of GET32 and PUT32.
//
// at runtime we do binary rewriting to replace every call to 
//  - GET32_inline with an equivalent load instruction
//  - and PUT32_inline with an equiv store instruction.
//
// we do it in this kind of roundabout way to make debugging easier.
// after you get this working, you can do something more aggressive
// as an extension!
#include "rpi.h"
#include "cycle-count.h"

// prototypes: these are identical to get32/put32.

// *(unsigned *)addr
unsigned GET32_inline(unsigned addr);
unsigned get32_inline(const volatile void *addr);

// *(unsigned *)addr = v;
void PUT32_inline(unsigned addr, unsigned v);
void put32_inline(volatile void *addr, unsigned v);

static int inline_on_p = 0, inline_cnt = 0;

// turn off inlining if we are dying.
#define die(msg...) do { inline_on_p = 0; panic("DYING:" msg); } while(0)

// inline helper: called from runtime-inline-asm.S:GET32_inline
// <lr> holds the value of the lr register, which will be 4 
// bytes past the call instruction to GET32_inline
// What this is doing!! :
// - inlining GET32 by replacing a function call with the 
// single load instruction that the function was conceptually doing anyway.
// - Inline_helper is a trampoline function. it will be called once. 
// - the next time execution reaches that same spot in the code, it no longer does the function call
uint32_t GET32_inline_helper(uint32_t addr, uint32_t lr) {
    // lr holds the instruction address that we will come back to after bl GET32_inline is done 
    if(inline_on_p) {
        // todo("smash the call instruction to just do: ldr r0, [r0]\n");
        // the address of the actual bl GET32_inline instruction
        uint32_t pc = lr - 4;

        // set the instruction at bl address to ldr instead. so we will not call functions but just load directly from memory
        *(volatile uint32_t *)pc = 0xe5900000;

        inline_cnt++;

        // turn inlining off while we print to make debugging easier.
        inline_on_p = 0;
        output("GET: rewriting address=%x, inline count=%d\n", pc, inline_cnt);
        inline_on_p = 1;
    }

    // manually do the load and return the first time.
    return *(volatile uint32_t*)addr;
}

// go through and rewrite similar to GET32_inline -- you have to 
// modify <runtime-inline-asm.S> as well.
void PUT32_inline_helper(uint32_t addr, uint32_t val, uint32_t lr) {
    if(inline_on_p) {
        uint32_t pc = lr - 4;
        *(volatile uint32_t *)pc = 0xe5801000;
        inline_cnt++;
        inline_on_p = 0;
        output("PUT: rewriting address=%x, inline count=%d\n", pc, inline_cnt);
        inline_on_p = 1;
    }
    // first call has to store here
    *(volatile uint32_t *)addr = val;
}

/********************************************************************
 * simple tests that check results.
 */

// this should get rerwitten
void test_get32_inline(unsigned n) {
    for(unsigned i = 0; i < n; i++) {
        uint32_t got = GET32_inline((uint32_t)&i);
        if(got != i) {
            inline_on_p = 0;
            panic("got %d, expected %d\n", got, i);
        }
    }
}

// this should not get rewritten initially.
void test_get32(unsigned n) {
    for(unsigned i = 0; i < n; i++) {
        uint32_t got = GET32((uint32_t)&i);
        if(got != i)
            panic("got %d, expected %d\n", got, i);
    }
}


// test using our runtime inline version.
void test_put32_inline(unsigned n) {
    uint32_t x;

    for(unsigned i = 0; i < n; i++) {
        PUT32_inline((uint32_t)&x, i);
        if(x != i)
            panic("got %d, expected %d\n", x, i);
    }
}

// test using regular put32: this won't get rewritten initially.
void test_put32(unsigned n) {
    uint32_t x;

    for(unsigned i = 0; i < n; i++) {
        PUT32((uint32_t)&x, i);
        if(x != i)
            panic("got %d, expected %d\n", x, i);
    }
}

/*************************************************************
 * versions without loops or checking: more sensitive to speedup
 */

// use our inline GET32: all of these calls should get inlined.
// a good check as an extension: after running this onece, check
// in GET32_inline that we never get called with an <lr> in this
// routine.
void test_get32_inline_10(void) {
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);

    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
    GET32_inline(0);
}

// use the raw GET32: these won't get inlined, which
// gives us a comparison point.
void test_get32_10(void) {
    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);

    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);
    GET32(0);
}

static volatile uint32_t put_test_var;

void test_put32_inline_10(void) {
    PUT32_inline((uint32_t)&put_test_var, 1);
    PUT32_inline((uint32_t)&put_test_var, 2);
    PUT32_inline((uint32_t)&put_test_var, 3);
    PUT32_inline((uint32_t)&put_test_var, 4);
    PUT32_inline((uint32_t)&put_test_var, 5);

    PUT32_inline((uint32_t)&put_test_var, 6);
    PUT32_inline((uint32_t)&put_test_var, 7);
    PUT32_inline((uint32_t)&put_test_var, 8);
    PUT32_inline((uint32_t)&put_test_var, 9);
    PUT32_inline((uint32_t)&put_test_var, 10);
}

// use the raw PUT32: these won't get inlined, which
// gives us a comparison point.
void test_put32_10(void) {
    PUT32((uint32_t)&put_test_var, 1);
    PUT32((uint32_t)&put_test_var, 2);
    PUT32((uint32_t)&put_test_var, 3);
    PUT32((uint32_t)&put_test_var, 4);
    PUT32((uint32_t)&put_test_var, 5);

    PUT32((uint32_t)&put_test_var, 6);
    PUT32((uint32_t)&put_test_var, 7);
    PUT32((uint32_t)&put_test_var, 8);
    PUT32((uint32_t)&put_test_var, 9);
    PUT32((uint32_t)&put_test_var, 10);
}

// we time things a bunch of different ways.
void notmain(void) {
    assert(!inline_cnt);

    output("about to test get32 inlining\n");
    inline_on_p = 1;
    uint32_t t_inline = TIME_CYC(test_get32_inline(1));
    uint32_t t_inline_run10 = TIME_CYC(test_get32_inline(100));
    uint32_t t_run10 = TIME_CYC(test_get32(100));
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);


    output("about to test get32 inlining without loop or checking\n");

    // test without a loop
    inline_on_p = 1;
    t_inline       = TIME_CYC(test_get32_inline_10());
    t_inline_run10 = TIME_CYC(test_get32_inline_10());
    t_run10        = TIME_CYC(test_get32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

    // todo("smash the original get32: should get same speedup\n");

    
    // smash the GET32 code to call the GET32_inline_helper, identically
    // how GET32_inline does.  you can' just copy the code since the branch
    // is relative to the destination.
    // Changes the actual machine code of GET32 itself!!

    uint32_t *get_pc = (void *)GET32;

    // 1. rewrite the GET32 code to call GET32_inline.
    get_pc[0] = 0xe1a0100e;
    uint32_t pc = (uint32_t)&get_pc[1];
    uint32_t target   = (uint32_t)GET32_inline_helper;
    uint32_t imm = ((target - (pc + 8)) >> 2) & 0x00ffffff;
    get_pc[1] = 0xea000000 | imm;
    // 2. this will result in all caller sites to GET32 getting
    //    inlined as well.
    // 3. thus: when it runs below, should have the same speedup
    

    output("after rewriting get32!\n");
    output("    inst[0] = %x\n", get_pc[0]);
    output("    inst[1] = %x\n", get_pc[1]);
    inline_on_p = 1;

    // this test should now have inline overhead.
    t_inline       = TIME_CYC(test_get32_10());
    // this is our original: should have the same speedup.
    t_inline_run10 = TIME_CYC(test_get32_inline_10());
    // after inlining this should be same as the GET32_inline overhead.
    t_run10        = TIME_CYC(test_get32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

    // 
    // PUT32
    // 
    output("about to test put32 inlining\n");
    inline_on_p = 1;
    t_inline = TIME_CYC(test_put32_inline(1));
    t_inline_run10 = TIME_CYC(test_put32_inline(100));
    t_run10 = TIME_CYC(test_put32(100));
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);


    output("about to test put32 inlining without loop or checking\n");

    // test without a loop
    inline_on_p = 1;
    t_inline       = TIME_CYC(test_put32_inline_10());
    t_inline_run10 = TIME_CYC(test_put32_inline_10());
    t_run10        = TIME_CYC(test_put32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

    
    // smash the GET32 code to call the GET32_inline_helper, identically
    // how GET32_inline does.  you can' just copy the code since the branch
    // is relative to the destination.
    // 
    //      note: 0xe1a0100e  =  mov r1, lr
    uint32_t *put_pc = (void *)PUT32;
    // orig_val0 = put_pc[0]; // original instruction at this address
    // orig_val1 = put_pc[1]; // next instruction 


    // 1. rewrite the PUT32 code to call PUT32_inline.
    put_pc[0] = 0xe1a0200e;
    pc = (uint32_t)&put_pc[1];
    target   = (uint32_t)PUT32_inline_helper;
    imm = ((target - (pc + 8)) >> 2) & 0x00ffffff;
    put_pc[1] = 0xea000000 | imm;
    // 2. this will result in all caller sites to GET32 getting
    //    inlined as well.
    // 3. thus: when it runs below, should have the same speedup
    

    output("after rewriting put32!\n");
    output("    inst[0] = %x\n", put_pc[0]);
    output("    inst[1] = %x\n", put_pc[1]);
    inline_on_p = 1;

    // this test should now have inline overhead.
    t_inline       = TIME_CYC(test_put32_10());
    // this is our original: should have the same speedup.
    t_inline_run10 = TIME_CYC(test_put32_inline_10());
    // after inlining this should be same as the GET32_inline overhead.
    t_run10        = TIME_CYC(test_put32_10());
    inline_on_p = 0;

    output("time to run w/ inlining overhead:   %d\n", t_inline);
    output("time to run 10 times inlined:       %d\n", t_inline_run10);
    output("time to run 10 times non-inlined:   %d\n", t_run10);
    output("total inline count=%d\n", inline_cnt);

}
