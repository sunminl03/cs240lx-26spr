#include "rpi.h"
#include "memtrace.h"

#include "watchpoint.h"

#include "mmu.h"
// 140e exception handling support
#include "full-except.h"
// 140e helpers for getting exception reason.
#include "armv6-except.h"
// 140e code for full context switching
// (caller,callee and cpsr).
#include "switchto.h"

#include "sbrk-trap.h"

// 1 = we expect a domain fault.
// 0 = we expect a or a watchpoint fault.
// used to catch some mistakes.
static int expect_domain_fault_p = 1;

// right now we only allow a single checker.  wrap this
// up for multiple checkers.
static memtrace_fn_t pre;
static memtrace_fn_t post;
static void *data;
// domain permisson enums: see b4-10
static int dom_trap = 0; 
enum { 
    DOM_no_access   = 0b00, // any access = fault.
    // client accesses check against permission bits in tlb
    DOM_client      = 0b01,
    // don't use.
    // DOM_reserved    = 0b10,
    // TLB access bits are ignored.
    DOM_manager     = 0b11,
    // pick some unused domain id.
    kern_dom = 1,
    
    // pre-compute the domain register values
    // that we need.
    //
    //  - <DOM_client> = hardware checks the page
    //    permissions.
    //  - each domain is 2 bits so we have to multiply
    //    by 2.

    // this only has the kernel domain: 
    // this will trap any heap acces.
    trap_heap_access = DOM_client << (kern_dom*2)
};

static int quiet_p = 0;
static int n_inst = 0;
void memtrace_yap_off(void) { quiet_p = 1; }
void memtrace_yap_on(void)  { quiet_p = 0; }

// pre-computed domain register values.
static uint32_t trap_access = 0;
static uint32_t no_trap_access = 0;

static int trap_is_on_p(void) {
    uint32_t v = domain_access_ctrl_get();
    return v == trap_heap_access;
}
static void trap_on(void) {
    domain_access_ctrl_set(trap_heap_access);

    // sanity check.  remove for speed.
    uint32_t v = domain_access_ctrl_get();
    assert(v = trap_heap_access);
}
static void trap_off(void) {
    int no_trap          = trap_heap_access 
                     |  DOM_client << (dom_trap*2);
    domain_access_ctrl_set(no_trap);

    // sanity check.  remove for speed.
    uint32_t v = domain_access_ctrl_get();
    assert(v = no_trap);
}

// turn memtracing on: wrapper with extra error checking.
void memtrace_trap_enable(void) {
    // need at least one handler!
    assert(pre || post);
    // if not true, didn't init
    assert(trap_access && no_trap_access);
    assert(!trap_is_on_p());
    trap_on();
}

// turn memtracing off: wrapper with extra error checking.
void memtrace_trap_disable(void) {
    // if not true, didn't init
    assert(trap_access && no_trap_access);
    assert(trap_is_on_p());
    trap_off();
}

// XXX: a good extension: change this so you look at the
// actual instruction and get the actual bytes.
//A5-30, A5-34
static inline unsigned inst_nbytes(uint32_t inst) {
    // byte vs word vs doubleword, half word ?
    int not_mis = ((inst >> 26) & 0b11) == 0x1;
    int byte = (inst >>22) & 1; 
    if (not_mis) {
        if (byte) {
            return 1;
        } else {
            return 4;
        }
    }

    int L = (inst >> 20) & 1;
    int S = (inst >> 6) & 1;
    int H = (inst >> 5) & 1;
    if ((!L && !S && H) || (L && !S && H) || (L && S && H)) {
        return 2;
    } else if ((!L && S && !H) || (!L && S && H)) {
        return 8;
    } else if (L && S && !H) {
        return 1;
    }
    
    return 4;
}

static void data_fault(regs_t *r) {
    // sanity check that we still at SUPER
    //   - should make it so we can run at user level.
    if(mode_get(r->regs[16]) != SUPER_MODE)
        panic("got a fault not at SUPER level?\n");
    // todo("handle the fault!");
    uint32_t reason     = data_abort_reason();
    uint32_t fault_addr = data_abort_addr();
    if (reason == DOMAIN_SECTION_FAULT) { // after a domain fault: call <pre>.
        // output("handling trap!\n"); 
        memtrace_trap_disable(); // do not want to trap before watchpoint gets triggered

        fault_ctx_t f = fault_ctx_mk(r, fault_addr, inst_nbytes(GET32(r->regs[15])), data_fault_from_ld()); 

        if (pre) {
            pre(data, &f);
        }
        watchpt_on_ptr((void*)fault_addr);
    } else if (watchpt_fault_p()) { // after a watchpoint fault: call <post>.
        // output("handling watchpoint!\n");
        uint32_t fault_pc   = watchpt_fault_pc();
        uint32_t fault_addr = watchpt_fault_addr();
        watchpt_off(fault_addr); // disable the watchpoint breakpoint
        n_inst++;
         uint32_t pc = r->regs[15];
        // r->regs[15] = pc;
        uint32_t r0 = r->regs[0];
        uint32_t r1 = r->regs[1];
        // trace("%d watchpoint fault:\n", n_inst);
        // output("    fault_addr=%x\n", fault_addr);
        // output("    fault_pc=%x\n", fault_pc);
        // output("    resume=[%x]\n", pc);
        // if(watchpt_load_fault_p()) {
        //     output("    load: GET32(%x) = %x, r0=%x\n", 
        //                 fault_addr,
        //                 GET32(fault_addr), 
        //                 r0);
        // } else {
        //     output("    store PUT32(%x) = %x  r0=%x, r1=%x\n", 
        //                 fault_addr,
        //                 GET32(fault_addr), 
        //                 r0, r1);
        // }
        fault_ctx_t f = fault_ctx_mk(r, fault_addr, inst_nbytes(GET32(r->regs[15])), data_fault_from_ld()); 
        f.pc = r->regs[15] - 4; 
        if (post) {
            post(data, &f);
        }
        memtrace_trap_enable(); // now remove access to the heap domain id
        while(!uart_can_put8())
            ;
    } else {
        panic("have a non-watchpt/trap fault?\n");
    }

    // drain printk to avoid the "can tx" race in UART.
    while(!uart_can_put8())
        ;

    switchto(r);
}

// initialize memtrace system.
void memtrace_init(
    void *data_h,
    memtrace_fn_t pre_h,
    memtrace_fn_t post_h,
    unsigned trap_dom) {

    // setting up VM does not belong here, but we do it to keep things
    // simple for today's lab.
    assert(!mmu_is_enabled());
    sbrk_init();
    assert(mmu_is_enabled());

    pre = pre_h;
    post = post_h;
    if(!pre && !post)
        panic("must supply one handler: pre=%x, post=%x\n", pre,post);
    data = data_h;
    assert(trap_dom < 16);
    trap_access = 1;
    no_trap_access = 1;
    dom_trap = trap_dom;

    // todo("do any additional setup you need");

    full_except_install(0);
    full_except_set_data_abort(data_fault);
}
