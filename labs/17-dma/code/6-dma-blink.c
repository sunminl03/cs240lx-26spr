// After 3-dma-blink, the only thing stopping us from doing the entire final for loop in DMA is that
// we couldn't do the `for(int i=0;i<N;i++)` in DMA. However, we just learned to do addition, so
// the only thing left is to figure out branching :)

#include "rpi.h"
#include "dma-impl.h"
#include "ctx.h"
#include "gpio-raw.h"
#include "pi-random.h"
void gpio_set_on_dma(dma_ch_t *dma, unsigned pin) {
    bus_t set0_bus_addr = arm_to_bus((uint32_t)GPIO_SET0);
    // static: DMA reads this asynchronously after dma_initiate returns
    static uint32_t pin_val;
    pin_val = 0x1u << pin;
    cb_t cb = cb_mk(set0_bus_addr, bus(&pin_val), 4);
    dma_run(dma, &cb, 1024);
}

void gpio_set_off_dma(dma_ch_t *dma, unsigned pin) {
    bus_t clr0_bus_addr = arm_to_bus((uint32_t)GPIO_CLR0);
    static uint32_t pin_val;
    pin_val = 0x1u << pin;
    cb_t cb = cb_mk(clr0_bus_addr, bus(&pin_val), 4);
    dma_run(dma, &cb, 1024);
}
// Feel free to put your own helpers in here :)
#define MHz 700UL

// convert nanoseconds to pi cycles: you need to modify this if you change
// the pi's clock speed.
#define ns_to_cycles(x) (unsigned) (((unsigned)x * 7UL) / 10UL )

static void ctx_gpio_on(ctx_t *ctx, unsigned pin) {
    static volatile uint32_t pin_val;
    pin_val = 0x1u << pin;
    ctx_emit_with(ctx, DST_INC | SRC_INC, arm_to_bus((uint32_t)GPIO_SET0), bus(&pin_val), 4);
}

static void ctx_gpio_off(ctx_t *ctx, unsigned pin) {
    static volatile uint32_t pin_val;
    pin_val = 0x1u << pin;

    ctx_emit_with(ctx,DST_INC | SRC_INC,arm_to_bus((uint32_t)GPIO_CLR0), bus(&pin_val), 4);
}

static void ctx_delay_ms(ctx_t *ctx, unsigned ms) {
    enum { CHUNK = 10000 };
    static uint32_t src[CHUNK/4];
    static volatile uint32_t dst;

    uint32_t total = (ms <= 2) ? 4 : (ms - 2) * 6000;
    while (total > 0) {
        uint32_t n = total > CHUNK ? CHUNK : total;

        ctx_emit_with(ctx, SRC_INC | (WAITS_MAX << WAITS_OFFSET), bus(&dst), bus(&src[0]), n);
        total -= n;
    }
}

static void ctx_noop(ctx_t *ctx) {
    static volatile uint8_t temp;
    // doing nothing 
    ctx_emit(ctx, bus(&temp), bus(&temp), 1);
}

void dma_delay_ms(dma_ch_t *dma, unsigned ms) {
    // SRC_INC reads through different cache lines (like part 2).
    // WAITS_MAX adds 28 - 33 so about 30 dummy cycles per bus transaction, slowing it further.
    enum { CHUNK = 10000 }; // 10000 bytes 
    static uint32_t src[CHUNK/4];
    static volatile uint32_t dst;

    // Starting estimate from part 2: (ms-2)*100 bytes. Recalibrate from output.
    uint32_t total = (ms <= 2) ? 4 : (ms - 2) * 6000;

    uint32_t remaining = total;
    while (remaining > 0) { //  loop reuses the same 10,000-byte buffer repeatedly
        uint32_t n;
        if (remaining > CHUNK) {
            n = CHUNK;
        } else {
            n = remaining;
        }
        cb_t cb = {};
        cb.TI = SRC_INC | (WAITS_MAX << WAITS_OFFSET);
        cb.SRC_ADDR = bus(&src[0]);
        cb.DST_ADDR = bus(&dst);
        cb.TXFR_LEN = n;
        // uint32_t t0 = cycle_cnt_read();
        if (!dma_run(dma, &cb, 100*1000*1000))
            panic("timed out\n");
        // uint32_t t1 = cycle_cnt_read();
        // output("cycles=%d for %d bytes\n", t1-t0, n);
        remaining -= n;
    }
}

cb_t *dma_blink(ctx_t *ctx, unsigned pin, unsigned delay, bus_t n_times) {
  // NOTE: n_times is a pointer! can't unroll

  // Goal: should be equivalent to the for(i=0;i<N;i++) { on ; delay ; off ; delay;  }
  //
  // It's useful to break this down to pseudo-assembly:
  //
  // loop:
  //     if i = N { goto end } <- this is the part you need to figure out
  //     i = i + 1
  //     on
  //     delay
  //     off
  //     delay
  //     goto loop <- you can use NEXT_CB to get this effect
  // end:
  //     exit <- this can be a no-op control block that has NEXT_CB=0
  // 
  // Useful for conditionals:
  //  - basic idea is https://en.wikipedia.org/wiki/Branch_table
  //  - similar to what you've done before, except instead of manipulating the SRC_ADDR of another
  //    block, you're manipulating the NEXT_CB!
  //  - you can assume n_times<256 if it helps
  //  - you might need to use multiple tables
  // todo("implement me!");
  static volatile _Alignas(256*256) uint8_t BRANCH_ADDR[256][256]; // i, N 
  static volatile _Alignas(256) uint8_t NEXTI[256]; 
  static volatile _Alignas(256) bus_t addrs[2];

  static volatile uint8_t i = 0; 
  
  for (int j = 0; j < 256; j++) {
    for (int n = 0; n < 256; n++) { // if i == N, then we go to END
      if (n == j) {
        BRANCH_ADDR[j][n] = 0; // access addr[0] that goes to exit
      } else {
        BRANCH_ADDR[j][n] = 4; // access addr[1] that goes to loop body
      }
    }
    NEXTI[j] = j+1; 
  }

  cb_t *first = ctx_here(ctx); // first control block. need to return it 
  bus_t loop = ctx_label(ctx); // the address of first control block 
  // loop: read i , read N, choose target, patch next cb
  ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop, 2, FLD_SRC, 0), n_times, 1);
  // dest is address of the array 
  ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop, 2, FLD_SRC, 1), bus(&i), 1);
  // dest is our control block's nnn block's source's 0 byte 
  ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop, 3, FLD_SRC, 0), bus(&BRANCH_ADDR[0][0]), 1);
  // so whether it is addrs[0] or addrs[1] depending on the value of branch_addr[i][j]
  // copy selected branch target address into branch_block.NEXT_CB
  ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop, 4, FLD_NXT, 0), bus(&addrs[0]), 4);

  // so after this branch block executes, we either go to body or exit 
  bus_t branch_block = ctx_label(ctx); // the real block 
  // branch_block :
  //    no-op
  //    exit 
  assert(ctx_at(ctx, branch_block));
  ctx_noop(ctx); // emits harmless cb. appended to linked list!

  bus_t body = ctx_label(ctx); // cb after noop 
  // body: 
  //      blah blah
  // emit 2 blocks
  ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(body, 1, FLD_SRC, 0), bus(&i), 1);
  ctx_emit(ctx, bus(&i), bus(&NEXTI[0]), 1);
  // light on off
  ctx_gpio_on(ctx, pin);
  ctx_delay_ms(ctx, delay);
  ctx_gpio_off(ctx, pin);
  ctx_delay_ms(ctx, delay);

  ctx_branch(ctx, loop); // go to loop 

  bus_t exit = ctx_label(ctx); 
  // exit: 
  //  do nothing
  ctx_noop(ctx);
  // setting exit's next cb to 0 
  ctx_end(ctx);

  addrs[0] = exit;
  addrs[1] = body;

  return first;




  // // we want next block to be that saved in addr[j][n]
  // let dma = dma_init(dma_ch);
  // gpio_set_on_dma(dma, pin);
  // dma_delay_ms(dma, 3000);
  // gpio_set_off_dma(dma, pin);
  // dma_delay_ms(dma, 3000);
  // // dest is the source of 
  // ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop_label, 1, FLD_SRC, 0), bus(&i), 1);
  // ctx_emit_with(ctx, DST_INC | SRC_INC, bus(&i), bus(&ADDR[0]), 1);
  // ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop_label, 3, FLD_SRC, 0), n_times, 1);
  // ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(loop_label, 3, FLD_SRC, 1), bus(&i), 1);
  // ctx_emit_with(ctx, DST_INC | SRC_INC, next, bus(&ADDR[0][0]), 1);
  

  // ctx_branch(ctx, next);

}

void notmain() {
  kmalloc_init();

  enum { DMA_CH = 4 };
  dma_ch_t *dma = dma_init(DMA_CH);

  ctx_t ctx;
  ctx_init(&ctx, 1024);

  enum { pin = 27, delay = 500 };
  gpio_set_output(pin);

  volatile uint8_t N = 5;
  cb_t *program_blink = dma_blink(&ctx, pin, delay, bus(&N));

  dma_initiate(dma, program_blink);
  // should take about 5s; round it up to 10
  enum { TIMEOUT_MICROS = 10 * 1000 * 1000 };
  uint32_t t_start = timer_get_usec();
  while(!dma_done(dma)) {
    if ((timer_get_usec() - t_start) >= TIMEOUT_MICROS)
      panic("dma timed out after %d seconds!\n", TIMEOUT_MICROS / 1000 / 1000);
  }
}
