// Starting on computation:
// use DMA to increment an 8-bit number

#include "rpi.h"
#include "dma-impl.h"
#include "ctx.h"

// Goal: a DMA chain that will do the following:
// 
//   *out = (*in + 1) % 256
// 
// <in> and <out> point to some locations in memory
//
// NOTES:
//  - it's called <inc8> because it increments an 8-bit value,
//    not because it adds 8 to a value
cb_t *stamp_inc8(ctx_t *ctx, bus_t out, bus_t in) {
  // Force EXAMPLE to be 256-byte-aligned.
  static volatile _Alignas(256) uint8_t EXAMPLE[256];
  // save all the mod 256 + 1 values in the array
  // ex. EXAMPLE[0] = 1
  for (int i = 0; i < 256; i++) {
    EXAMPLE[i] = i+1; 
    if (i == 255) {
      EXAMPLE[i] = 0; 
    }
  }
  // Ex. if *in == 2, we want EXAMPLE[2] 
  // want to get EXAMPLE[*in]. memcpy in into the last byte of the address of EXAMPLE


  // Useful:
  //  - Can use _Alignas(1<<k) to put variables at addresses
  //    that have their lowest k bits zeroed (See the EXAMPLE
  //    variable up above)
  //  - inc8 can be done pretty easily with just raw cb_t
  //    manipulation, but would recommend using the `ctx_*`
  //    APIs or starting to make your own helpers
  //  - also: highly recommend checking out e.g. <bus_rel_fld>
  //  - SRC_ADDR is just 4 bytes in memory

  // drop this if you're not using the ctx_* stuff
  bus_t cb1_label = ctx_label(ctx);
  cb_t *inc8_first_cb_addr = ctx_here(ctx);
  //
  // destination is cb1_label's next block's 0th byte of the source addr
  // we are memcpying "in" into the last byte of EXAMPLE's addr 
  // then copy the content of that address to "out"
  ctx_emit_with(ctx, DST_INC | SRC_INC, bus_rel_fld(cb1_label, 1, FLD_SRC, 0), in, 1);
  ctx_emit_with(ctx, DST_INC | SRC_INC, out, bus(&EXAMPLE[0]), 1);

  // we cannot do sth like 
  // ctx_emit(ctx, bus(&example_addr), in, 1);
  // ctx_emit(ctx, out, example_addr, 1);
  // because the second control block has already been created with its own SRC_ADDR field set to the old value
  // so we need to make it relative.
 
  

  // todo("implement me!");

  // Note: if you're using ctx_*, then at some point you need
  // to set the NEXT_CB of the last block you emitted to 0.
  //
  // You do this by calling ctx_end(); right now, we're doing
  // that in <notmain> after <stamp_inc8> returns.

  return inc8_first_cb_addr;
}

void notmain() {
  kmalloc_init();

  enum {
    DMA_CH = 4,
  };
  dma_ch_t *dma = dma_init(DMA_CH);

  // Allocate an arena of 1024 blocks (if using ctx)
  ctx_t ctx;
  ctx_init(&ctx, 1024);

  volatile uint8_t out, in;
  cb_t *program_inc8 = stamp_inc8(&ctx, bus(&out), bus(&in));
  // set the NEXT_CB of the last block to 0
  // drop if not using ctx_*
  ctx_end(&ctx);


  for (int i = 0; i < 256; i++) {
    enum {
      TIMEOUT = 1024,
    };
    out = 0;
    in = i;
    dma_initiate(dma, program_inc8);
    if (!dma_wait(dma, TIMEOUT))
      panic("dma timed out\n");

    if (out != (uint8_t)(in + 1) || in != i)
      panic("didn't increment: out=%x, in=%x expected=%x\n", out, in, in + 1);
  }

  output("SUCCESS!  8-bit increment works on all inputs!\n");
}
