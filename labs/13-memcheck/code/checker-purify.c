// engler: cs240lx: a sort-of purify checker: gives an error
// message if a load/store to a heap addres is:
//   - not within a legal block.
//   - to freed memory.
//
// uses the checking allocator (ckalloc).  for the moment
// just reboot on an error.
// 
// limits:
//   - does not check that its within the *correct*
//     legal block (can do this w/ replay).
//   - does not track anything about global or stack memory.
#include "memtrace.h"
#include "ckalloc.h"
#include "purify.h"
#include "sbrk-trap.h"
#include "memmap-default.h"

static int purify_quiet_p = 0;
void purify_yap_off(void) {
    purify_quiet_p = 1;
    memtrace_yap_off();
}
void purify_yap_on(void) {
    purify_quiet_p = 0;
    memtrace_yap_on();
}

// turn off trapping when we allocate.  Q: what if we 
// don't do this?
void *purify_alloc_raw(unsigned n, src_loc_t l) {
    memtrace_trap_disable();

        // if shadow memory: mark [p,p+n) as allocated
        unsigned *p = (ckalloc)(n, l);

    memtrace_trap_enable();
    return p;
}

// turn off trapping when we free.  Q: what if we 
// don't do this?
void purify_free_raw(void *p, src_loc_t l) {
    memtrace_trap_disable();

        // if shadow memory: mark [p,p+n) as free
        (ckfree)(p, l);

    memtrace_trap_enable();
}
void  purify_error(hdr_t *hdr, void *addr) {
    int offset = ck_illegal_offset(hdr, addr);
    char s[20];
    if (hdr->state == FREED) {
        memcpy(s, "FREED", 10);
    } else {
        memcpy(s, "allocated", 15);
    }
    if (offset < 0) {
        ck_error(hdr, " illegal store to to %s block at :  is %d bytes before legal mem (block size=%d)\n", s, offset, hdr->nbytes_alloc); 
    } else if (offset > 0) {
        ck_error(hdr, " illegal store to to %s block at :  is %d bytes after legal mem (block size=%d)\n", s, offset, hdr->nbytes_alloc); 
    } else {
        ck_error(hdr, " use after free at : illegal store to to  within freed block\n");
    }
    clean_reboot();
}
static int purify_handler(void *data, fault_ctx_t *f) {
    // todo("implement this code!  if error: reboot");
    uint32_t mem_addr = f->addr; // mem addr of fault 
    int load_p = f->load_p;
    if (load_p) {
        trace(": load from address %x\n", mem_addr);
    } else {
        trace(": store to address %x\n", mem_addr);
    }
    hdr_t * hdr = ck_ptr_is_alloced((void *)mem_addr); 
    if (hdr != NULL) {
        return MEMTRACE_OK;
    } else {
        hdr_t * invalid_hdr = ck_get_containing_blk((void *) mem_addr); 
        purify_error(invalid_hdr, (void *)mem_addr);
    }
    return MEMTRACE_OK;
}

void purify_init(void) {
    memtrace_init(0, purify_handler, 0, dom_trap);
    memtrace_trap_enable();
    memtrace_yap_off();
}
