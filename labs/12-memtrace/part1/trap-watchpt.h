
// enum { 
//     // pick some unused domain id.
//     kern_dom = 1,
//     heap_dom = 2,
    
//     // pre-compute the domain register values
//     // that we need.
//     //
//     //  - <DOM_client> = hardware checks the page
//     //    permissions.
//     //  - each domain is 2 bits so we have to multiply
//     //    by 2.

//     // this only has the kernel domain: 
//     // this will trap any heap acces.
//     trap_heap_access = DOM_client << (kern_dom*2),

//     no_trap          = trap_heap_access 
//                      |  DOM_client << (heap_dom*2)
// };
#include "switchto.h"
// start trapping heap accesses by switching the
// domain register.
void memtrace_trap_enable(void);

// turn heap-trapping off.
void memtrace_trap_disable(void);

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
static int vm_map_everything(void);

static uint32_t expected_fault_addr = 0;
static uint32_t expected_fault_pc = 0;
// used to turn off output (1=no output).
static volatile int n_inst = 0;


void memtrace_handler(regs_t *r);

// we don't expect prefetch faults for this code.
static void prefetch_fault(regs_t *r);
// void memtrace_handler(regs_t *r, uint32_t fault_addr, int load_p) {

// }

//  That will do a one-time initialization of everything (VM, heap,
//  exception handlers) that is needed.  You can just steal all the
//  code for this from the examples.  In a real system these pieces
//  would be sharded-out in a better way, but for now we cut corners.
void memtrace_init(void);

