#include "rpi.h"
#include "rpi-pmu.h"


// no optimization
__attribute__((noinline,optimize("O0")))
static uint32_t no_opt_sum(void) {
    uint32_t sum = 0;

    for(unsigned i = 0; i < 100; i++)
        sum += i;

    return sum;
}

// using file's normal -O2, GCC will prob optimize and give fewer cycles?
__attribute__((noinline))
static uint32_t opt_sum(void) {
    uint32_t sum = 0;

    for(unsigned i = 0; i < 100; i++)
        sum += i;

    return sum;
}

static void measure_sum(const char *msg, uint32_t (*fn)(void),
                        uint32_t *n_inst, uint32_t *n_branch) {
    uint32_t inst, branch = 0;

    pmu_stmt_measure_set(inst, branch,
        msg,
        inst_cnt, branch_cnt,
        {
            uint32_t ret = fn();
        });

    *n_inst = inst;
    *n_branch = branch;
}

void notmain(void) {
    pmu_on();
    caches_enable();

    uint32_t no_opt_inst, no_opt_branch;
    uint32_t opt_inst, opt_branch;
    // seeing if GCC optimization reduces cycles
    measure_sum("forced O0: no opt",no_opt_sum, &no_opt_inst, &no_opt_branch);
    measure_sum("normal O2: loop might be optimized", opt_sum, &opt_inst, &opt_branch);

    output("summary: O0 inst=%d branch=%d; O2 inst=%d branch=%d\n", no_opt_inst, no_opt_branch, opt_inst, opt_branch);
}
