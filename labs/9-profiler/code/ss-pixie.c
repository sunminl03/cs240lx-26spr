// a simple single step tracing example.
//
// lab: 
//   1. change this code so it is an instruction profiler 
//      (similar go gprof in 140e)
//   2. add PMU counters to make things more precise.
//      (you will have to change the <prefetch_abort_vector>
//      signature)
#include "rpi.h"
#include "ss-pixie.h"

#include "breakpoint.h"
#include "cpsr-util.h"
#include "memmap.h"

// TODO: array to hold instruction count 
static unsigned hist_n, pc_min, pc_max;
static volatile unsigned *hist = 0;

static volatile unsigned *hist_cnt = 0;


// counter of the number of instructions.
static volatile unsigned n_inst = 0;

// use a swi instruction to invoke system
//  see: <ss-pixie-asm.S>
int pixie_invoke_syscall(int syscall, ...);

// switch to SUPER mode and load <regs>
//  see: <ss-pixie-asm.S>
void pixie_switchto_super_asm(uint32_t regs[16]);

// switch to USER mode.
//  see: <ss-pixie-asm.S>
void pixie_switchto_user_asm(void);

// control output a bit: if non-zero print, otherwise don't.
#define pixout(args...) \
    do { if(pixie_verbose_p) output(args); } while(0)

static int pixie_verbose_p = 1;
void pixie_verbose(int verbose_p) {
    pixie_verbose_p = verbose_p;
}

// called on each single-step exception. see
// <ss-pixie-asm.S>
void prefetch_abort_vector(uint32_t pc) {
    uint32_t start_cnt, end_cnt;
    asm volatile("MRC p15, 0, %0, c13, c0, 2" : "=r"(start_cnt)); // start cycle count of current instruction
    // trace("start_cnt = %u\n", start_cnt);
    asm volatile("MRC p15, 0, %0, c13, c0, 3" : "=r"(end_cnt)); // the end cycle count of previous instruction
    // trace("end_cnt = %u\n", end_cnt);

    if(!brkpt_fault_p())
        panic("have a non-breakpoint fault\n");

    n_inst++;
    pixout("%d: ss fault: pc=%x\n", n_inst, pc);
    // TODO: In the fault handler, use the program counter value (register 15) to index into this array and increment the associated count.
    uint32_t index = (pc-pc_min) >> 2;
    hist[index] += 1;
    if (index > 0) {
        hist_cnt[index - 1]+= (start_cnt - end_cnt);
    }

    // set a mismatch on the fault pc so that we can run it.
    brkpt_mismatch_set(pc);

    // if you don't print: you don't have to do this.
    // if you do print, there is a race condition if we 
    // are stepping through the UART code --- note: we could
    // just check the pc and the address range of
    // uart.o
    while(!uart_can_put8())
        ;
}

// called before dying.  we make the symbol weak so
// that if the test case has, will override.
// this lets test cases do thier own internal
// checking.
void WEAK(pixie_die_handler)(uint32_t regs[16]) {
    output("done: dying!\n");
}

// very limited system calls.  we only need these
// b/c SS has to run at USER level so can't switch
// back.
void pixie_syscall(int sysno, uint32_t *regs) {
    // check the saved spsr and make sure is USER level.
    if(mode_get(spsr_get()) != USER_MODE)
        panic("not at USER level?\n");

    switch(sysno) {
    case PIXIE_SYS_DIE:
        pixie_die_handler(regs);
        clean_reboot();
    case PIXIE_SYS_STOP:
        pixout("done: pc=%x\n", regs[15]);
        // change to SUPER
        pixie_switchto_super_asm(regs);
        not_reached();
    default:
        panic("invalid syscall: %d\n", sysno);
    }
    not_reached();
}

// defines <cp_asm_set> macro.
#include "asm-helpers.h"

// 3-121 --- set exception jump table location.
cp_asm_set(vector_base_asm, p15, 0, c12, c0, 0)


// start single step mismatching.
//  1. sets up exception vectors (idempotent).
//  2. enables single stepping (idempotent)
//  3. switches to USER mode.
//
// NOTE:
//   if you want to ignore instructions until 
//   return back outside of <pixie_start>
//   1. get the return address before switching
//      to USER mode
//         ignore_until_pc = __builtin_return_address(0);
//   2. in the single-step handler: ignore all PC values 
//      until <pc> equals <ignore_until_pc>
void pixie_start(void) {
    // vector of exception handlers: see <ss-pixie-asm.S>
    // check alignment and then set the vector base to it.
    extern uint32_t pixie_exception_vec[];
    uint32_t vec = (uint32_t)pixie_exception_vec;
    unsigned rem = vec % 32;
    if(rem != 0)
        panic("interrupt vec not aligned to 32 bytes!\n", rem);
    vector_base_asm_set(vec);
    
    // TODO: allocate a table with one entry for each instruction
    // Use kmalloc (or, better: your ckalloc!) to allocate an array at least as big as the code. 
    // (If you use kmalloc make sure to do kmalloc_init first.) 
    // Compute the code size using the labels defined in libpi/memmap 
    // (we give C definitions in libpi/include/memmap.h)
    uint32_t oneMB = 1024*1024;
    kmalloc_init_set_start((void*)oneMB, oneMB);
    pc_min =(unsigned int) __code_start__;
    pc_max = (unsigned int) __code_end__;
    hist_n = (pc_max - pc_min) >> 2;
    hist = kmalloc(hist_n * sizeof(hist[0]));
    hist_cnt = kmalloc(hist_n * sizeof(hist[0]));

    // enable mismatching.  note: we are currently
    // at privileged mode so won't start til we 
    // switch to to user mode.
    //
    // this routine is from 140e.  will: 
    //  1. enable the debug co-processor cp14 
    //  2. set a mismatch on address <0>.  once we are 
    //    at user-level, this will cause any pc != 0 (all 
    //    of them) to immediately mismatch
    brkpt_mismatch_start();

    // switch from SUPER to USER mode: not just a matter of
    // switching the cpsr since the sp,lr are different.

    // 1. sanity check that we are at SUPER_MODE
    if(mode_get(cpsr_get()) != SUPER_MODE)
        panic("not at SUPER level?\n");

    // 2. switch to USER.  will immediately
    // immediately start mismatching when
    // the mode switch occurs in routine. 
    pixie_switchto_user_asm();

    // 3. sanity check that at USER. 
    //
    // NOTE: this check adds extra instructions to the 
    // trace if you don't filter them (see note above)
    if(mode_get(cpsr_get()) != USER_MODE)
        panic("not at user level?\n");
}

// turn off single step matching. 
//
// we need to do a system call b/c we are running 
// at user level and thus can't write to the debug 
// co-processor
//
// alernative: catch the undefined instruction and 
// do it.
unsigned pixie_stop(void) {
    pixie_invoke_syscall(PIXIE_SYS_STOP);
    pixout("pixie: stopped!\n");
    return n_inst;
}

void pixie_dump(unsigned N) {
    // todo("build this");
    for (int i = 0; i < hist_n; i++) {
        if (hist[i] > N) {
            printk("pc=%x count=%u average cycles per instruction=%u\n", pc_min + (i << 2), hist[i],  hist_cnt[i]/hist[i]);
        }
        // printk("pc= %x average cycles per instruction=%u\n", pc_min + (i << 2), hist[i], hist_cnt[i]/hist[i]);
    }
}

void pixie_reset(void) {
    for (int i = 0; i < hist_n; i++) {
        hist[i] = 0;
        hist_cnt[i] = 0;
    }
}