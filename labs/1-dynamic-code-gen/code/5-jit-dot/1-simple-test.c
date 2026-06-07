// simple test of dot product jitting.
#include "jit-dotproduct.h"
#include "armv6-pmu.h"

void notmain(void) { 
    jit_init();

    enum { 
        ntrials = 8,       // number of trials
        n = 8,             // size of vector
        percent_0 = 20,    // percentage 0s
        verbose_p = 1,
        nruns = 1000       // repeat count for PMU measurement
    };

    for(unsigned i = 0; i < ntrials; i++) {
        uint32_t *a = vec_mk(n,0);
        uint32_t *b = vec_mk(n,percent_0);

        if(verbose_p) {
            vec_print("A", a, n);
            vec_print("B", b, n);
        }

        uint32_t d0 = vec_dot(a,b,n);
        vec_fn_t dot_fn = jit_dot(b,n);
        uint32_t d1 = dot_fn(a);

        if(d0 != d1)
            panic("static dot=%d, jit dot=%d\n", d0,d1);

        if(verbose_p) 
            output("passed: static dot = jit dot = %d\n", d0);

       
        // EXTENSTION
        // baseline measurement
        pmu_stmt_measure("vec_dot: inst + branch",
            inst_cnt,
            branch_cnt, {
                volatile uint32_t x = 0;
                for(unsigned j = 0; j < nruns; j++)
                    x += vec_dot(a,b,n);
            });

        // jit measurement
        pmu_stmt_measure("jit dot: inst + branch",
            inst_cnt,
            branch_cnt, {
                volatile uint32_t x = 0;
                for(unsigned j = 0; j < nruns; j++)
                    x += dot_fn(a);
            });
    }

    output("--------------------------------------------\n");
    output("n=%d, percent zero=%d, ntrials=%d\n",
        n, percent_0, ntrials);
}