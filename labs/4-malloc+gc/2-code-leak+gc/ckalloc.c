// implement a simple ckalloc/free that adds ckhdr_t to the 
// allocation.
#include "rpi.h"
#include "ckalloc.h"
#include "kr-malloc.h"

unsigned ck_verbose_p = 0;
int total_bytes_allocated = 0;
// should hold all allocated blocks
static hdr_t *alloc_list; 
// returns pointer to the first allocated header block.
hdr_t *ck_first_alloc(void) {
    return alloc_list;
}

// return header associated with <ptr> if one exists.
hdr_t *ck_ptr_is_alloced(void *ptr) {
    for(hdr_t *h = ck_first_alloc(); h; h = ck_next_hdr(h)) {
        // output("hi. this is header addr = %x\n", h);

        if(ck_ptr_in_block(h,ptr)) 
            return h;
    }
    return 0;
}


/***********************************************************************
 * implement the rest
 */

// is <ptr> inside <h>'s data block?
unsigned ck_ptr_in_block(hdr_t *h, void *ptr) {
    if(h->state != ALLOCED) {
        panic("should only have allocated blocks: state=%d, blockid =%d", h->state, h->block_id);
        return 0;
    }

    // use ck_data_start/_end 
    // we are scanning the addresses of each byte of the allocated payload. 
    // If the pointer is equivalent to one of the addresses, then we say that it is "in the block"
    int* start = (int *)ck_data_start(h); // pointer to the start of allocated data
    int* end = (int *)ck_data_end(h);
    for (char* i = (char *)start; i < (char *)end; i++) { // should  be char * so we scan byte by byte 
        if (i == (char *)ptr) {
            return 1;
        }
    }
    return 0;
    // todo("check that <ptr> is in data for <h>\n");
}


// free a block allocated with <ckalloc>
void (ckfree)(void *addr, src_loc_t l) {
    hdr_t *h = ck_ptr_is_alloced(addr);
    if(!h)
        loc_panic(l, "freeing bogus pointer: %p\n", addr);

    // allocated block starts right after the header.

    void *blk_start = ck_data_start(h); // addr should be the start of data
    if(blk_start != addr)
        loc_panic(l, "not freeing using start pointer: have %p, need %p\n",
            addr, blk_start);

    if(h->state != ALLOCED)
        loc_panic(l, "freeing unallocated memory: state=%d\n", h->state);
    if(ck_verbose_p)
        loc_debug(l, "freeing %p\n", addr);

    assert(ck_ptr_is_alloced(addr));
    h->state = FREED;
    // output("inside free\n");
    // just remove from the allocated list.
    // todo("implement the rest\n");
    hdr_t *preh = NULL;
    for (hdr_t *curh = ck_first_alloc(); curh; curh = ck_next_hdr(curh)) {
        // output("inside for loop\n");
        if (curh == h) {
            if (curh == ck_first_alloc()) {
                // output("first one allocated\n");
                alloc_list = curh->next;
            } else {
                // output("not the first one allocated\n");
                preh->next = curh->next;
            }
            break;
        }
        preh = curh;
    }
    total_bytes_allocated -= h->nbytes_alloc;
    kr_free(h);
}


// interpose on kr_malloc allocations and
//  1. allocate enough space for a header and fill it in.
//  2. add the allocated block to  the allocated list.
void *(ckalloc)(uint32_t nbytes, src_loc_t l) {
    static unsigned block_id=1;

    hdr_t *h = kr_malloc(nbytes + sizeof *h);

    memset(h, 0, sizeof *h);
    h->nbytes_alloc = nbytes;
    h->state = ALLOCED;
    h->alloc_loc = l;
    h->block_id = block_id++;
    // trace("allocated block id = %d", h->block_id);

    // set addr;
    void *addr = 0;
    // output("addr is %x", addr);

    // todo("put on allocated list\n");
    int cnt = 0;
    if (total_bytes_allocated == 0) {
        alloc_list = h;
    } else {
        hdr_t *curh = alloc_list;
        hdr_t *preh = alloc_list;
        while (cnt < total_bytes_allocated) {
            cnt += curh->nbytes_alloc;
            preh = curh;
            curh = curh->next; 
        }
        // last header of the allocated list should point to this header 
        preh->next = h;
    }
    // increment nbytes_alloced
    total_bytes_allocated += nbytes;
    addr = ck_data_start(h);
    // output("addr is %x", addr);
    assert(ck_ptr_is_alloced(addr));
    if(ck_verbose_p)
        loc_debug(l, "successful alloc of %p\n", addr);
    return addr;
}
