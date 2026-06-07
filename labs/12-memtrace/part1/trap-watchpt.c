// a simple example of how to trap and emulate
// memory operations for a given memory regions
// by using ARMv6 domain fault trapping.
//
// we want to do trap-and-emulate so we can make 
// memory tools easily.  these tools require:
//   1. trapping all loads and stores before they 
//      occur.
//   2. inspecting them (eg to check if 
//      the address was illegal).  
//   3. if the access was bad: emit an error message.
//   4. otherwise perform the memory instruction 
//      and continue.
//
// using some clever ARM hardware tricks we can
// do this pretty easily.
// 
// the basic idea: given a memory region M we want
// to trap:
//   1. tag all of M's virtual memory mappings 
//      with its own domain id <d_trap> (ARMv6 has 
//      16 domains).
//   2. when we want to trap, remove permissions
//      for the <d_trap> domain.
//   3. when we want turn off trapping, add 
//      permissions for the <d_trap> domain.
//   4. when we get a fault and want to emulate
//      the memory instruction:
//        A. disable trapping so we can access the 
//           memory.
//        B. perform the memory op
//        C. enable trapping.
//        D. jump back to the next instruction.
//
// the only painful part of this on the arm is
// step 4.B: performing the memory op since
// since ARM has a huge number of memory ops.
// thus, *for this example code* we cheat by 
// forcing the client to access trapping memory 
// with either PUT32 or GET32 because:
//    1. they use ldr (GET32) or str (PUT32)
//    2. so it's easy to emulate them. 
//
// you can see this by looking at any .list file:
//  0000803c <PUT32>:
//      803c:   e5801000    str r1, [r0]
//      8040:   e12fff1e    bx  lr
//
//  00008044 <GET32>:
//      8044:   e5900000    ldr r0, [r0]
//      8048:   e12fff1e    bx  lr
//
// The lab removes this restriction by using 
// single-stepping to make emulating any 
// memory operation easy (by just running it)
#include "rpi.h"
#include "trap-watchpt.h"
#include "watchpoint.h"

// 140e code for doing virtual memory using
// TLB pinning.
#include "pinned-vm.h"
// 140e exception handling support
#include "full-except.h"
// 140e helpers for getting exception reason.
#include "armv6-except.h"
// 140e code for full context switching
// (caller,callee and cpsr).
#include "switchto.h"

// default definitions for how address space
// is laid out.
#include "memmap-default.h"

enum { 
    // pick some unused domain id.
    kern_dom = 1,
    heap_dom = 2,
    
    // pre-compute the domain register values
    // that we need.
    //
    //  - <DOM_client> = hardware checks the page
    //    permissions.
    //  - each domain is 2 bits so we have to multiply
    //    by 2.

    // this only has the kernel domain: 
    // this will trap any heap acces.
    trap_heap_access = DOM_client << (kern_dom*2),

    no_trap          = trap_heap_access 
                     |  DOM_client << (heap_dom*2)
};

// start trapping heap accesses by switching the
// domain register.
void memtrace_trap_enable(void) {
    domain_access_ctrl_set(trap_heap_access);

    // sanity check.  remove for speed.
    uint32_t v = domain_access_ctrl_get();
    assert(v = trap_heap_access);
}

// turn heap-trapping off.
void memtrace_trap_disable(void) {
    domain_access_ctrl_set(no_trap);

    // sanity check.  remove for speed.
    uint32_t v = domain_access_ctrl_get();
    assert(v = no_trap);
}

// we need virtual memory for trapping.  so setup the
// simplest possible VM: 
//   identity mapping of only the 1mb sections used by 
//   our basic process (code, data, heap, stack and 
//   exception stack).  
// we pin these entries in the tlb so we don't even 
// need a page table.  
//
// we tag the heap with its own domain id (<heap_dom>), 
// and everything else with a different one <kern_dom>
//
// to keep things simple, we specialize this to what we
// need with our simple memtrap tests.
//
static int vm_map_everything(void) {
    // initialize the hardware MMU for pinned vm
    pin_mmu_init(no_trap);
    assert(!mmu_is_enabled());


    // compute the different mapping attributes.  
    // we only do simple uncached mappings today
    // (but shouldn't matter).

    // device memory: kernel domain, no user access, 
    // memory is strongly ordered, not shared.
    // we use 16mb section.
    pin_t dev  = pin_16mb(pin_mk_global(kern_dom, no_user, MEM_device));

    // kernel memory: same as device, but is only uncached.  
    pin_t kern = pin_mk_global(kern_dom, no_user, MEM_uncached);

    // heap.  different from kernel memory b/c:
    // 1. needs a different domain so will trap.
    // 2. user_access: since when we add single stepping 
    //    the code will run at user level.  (alternatively
    //    we could set <heap_dom> to manager permission)
    pin_t heap = pin_mk_global(heap_dom, user_access, MEM_uncached);

    // now identity map kernel memory.
    unsigned idx = 0;
    pin_mmu_sec(idx++, SEG_CODE, SEG_CODE, kern);

    // we could mess with the alignment to give the
    // heap more memory.
    pin_mmu_sec(idx++, SEG_HEAP, SEG_HEAP, heap);
    pin_mmu_sec(idx++, SEG_STACK, SEG_STACK, kern);
    pin_mmu_sec(idx++, SEG_INT_STACK, SEG_INT_STACK, kern);
    pin_mmu_sec(idx++, SEG_BCM_0, SEG_BCM_0, dev);

    // we aren't using user processes or anythings so we
    // just claim ASID=1 as our address space identifier.
    enum { ASID = 1 };
    pin_set_context(ASID);

    // turn the MMU on.
    assert(!mmu_is_enabled());
    mmu_enable();
    assert(mmu_is_enabled());
    // vm is now live!

    // return index in case if want to allocate more.
    return idx;
}

// static uint32_t expected_fault_addr = 0;
// static uint32_t expected_fault_pc = 0;
// // used to turn off output (1=no output).
// static volatile int n_inst = 0;


void memtrace_handler(regs_t *r){
    // b4-43 [140e pinned mem]
    uint32_t reason     = data_abort_reason();
    // b4-44 [140e pinned mem]
    uint32_t fault_addr = data_abort_addr();

    // b4-20 has the different reasons.
    if(reason == DOMAIN_SECTION_FAULT) { // if aborted because domain trapped 
        output("handling trap!\n"); 
         // turn this off so the code can read or write the heap.
        memtrace_trap_disable();
        // Set a watchpoint fault on the address that caused the domain fault.
        watchpt_on_ptr((void*)fault_addr); // address that was accessed and caused fault

        expected_fault_addr = (uint32_t)fault_addr;
        expected_fault_pc = (uint32_t)r->regs[15];
        switchto(r); // go back and execute this instruction again
    }
    else if (watchpt_fault_p()) {// if aborted because watchpoint
        output("handling watchpoint!\n");
        uint32_t fault_pc   = watchpt_fault_pc();
        uint32_t fault_addr = watchpt_fault_addr();
        if(fault_addr != expected_fault_addr)
        panic("expected a fault on %x, have %x\n", 
            expected_fault_addr, 
            fault_addr);

        if(fault_pc != expected_fault_pc)
            panic("expected a fault at pc=%x, have pc=%x\n", 
                expected_fault_pc,
                fault_pc);

        watchpt_off(fault_addr); // disable the watchpoint breakpoint
        n_inst++;
        uint32_t pc = r->regs[15];
        // r->regs[15] = pc;
        uint32_t r0 = r->regs[0];
        uint32_t r1 = r->regs[1];
        trace("%d watchpoint fault:\n", n_inst);
        output("    fault_addr=%x\n", fault_addr);
        output("    fault_pc=%x\n", fault_pc);
        output("    resume=[%x]\n", pc);
        if(watchpt_load_fault_p()) {
            output("    load: GET32(%x) = %x, r0=%x\n", 
                        fault_addr,
                        GET32(fault_addr), 
                        r0);
        } else {
            output("    store PUT32(%x) = %x  r0=%x, r1=%x\n", 
                        fault_addr,
                        GET32(fault_addr), 
                        r0, r1);
        }
        memtrace_trap_enable(); // now remove access to the heap domain id
        // drain printk if neeed.
        while(!uart_can_put8())
            ;

        // resume by loading all registers.
        switchto(r);
    } else {
        panic("have a non-watchpt/trap fault?\n");
    }
}

// we don't expect prefetch faults for this code.
static void prefetch_fault(regs_t *r) {
    panic("we got a prefetch abort fault at pc=%x\n", r->regs[15]);
}

// void memtrace_handler(regs_t *r, uint32_t fault_addr, int load_p) {

// }

//  That will do a one-time initialization of everything (VM, heap,
//  exception handlers) that is needed.  You can just steal all the
//  code for this from the examples.  In a real system these pieces
//  would be sharded-out in a better way, but for now we cut corners.
void memtrace_init(void) {
    // our kmalloc standard init.
    kmalloc_init_set_start((void*)SEG_HEAP, MB(1));

    // setup the full fault handlers [140e] that take in
    // the full register structure --- all 16 general
    // registers and the cpsr  --- that were live at the 
    // fault.
    full_except_install(0);
    full_except_set_data_abort(memtrace_handler);
    full_except_set_prefetch(prefetch_fault);
    // exception handler for watchpt
    // full_except_set_data_abort(watchpt_handler);

    // map everything: when this returns vm is on!
    int idx = vm_map_everything();
    assert(mmu_is_enabled());

    // get the current domain.
    let x = domain_access_ctrl_get();
    trace("%d total mappings, domain = %b\n", idx, x);
}

