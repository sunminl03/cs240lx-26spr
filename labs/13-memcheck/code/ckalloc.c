// implement a simple ckalloc/free that adds ckhdr_t to the 
// allocation.
#include "rpi.h"
#include "ckalloc.h"


// gcc will happily instrument its own instrumentation code, smh.
void __attribute__((no_instrument_function)) 
__cyg_profile_func_enter(void *this_fn, void *call_site);
void __attribute__((no_instrument_function)) 
__cyg_profile_func_exit(void *this_fn, void *call_site);

static volatile int depth = 0, can_print_p = 0;;
// also don't instrument this helper.
__attribute__((no_instrument_function)) 
static inline void indent(int n) {
    while(n-- > 0)
        output(" ");
}

static alloc_t alloc_fn = kmalloc;
static free_t free_fn = 0;
    
void ckalloc_init(alloc_t allocfn, free_t freefn) {
    assert(allocfn);
    alloc_fn = allocfn;
    free_fn = freefn;
}

void __cyg_profile_func_enter(void *this_fn, void *call_site) {
    // haven't started tracing yet.
    if(!can_print_p)
        return;
    depth++;

    can_print_p = 0;
    indent(depth*5); output("entering %x\n", this_fn);
    can_print_p = 1;
}
 
// — Called just before exiting a function. ￼
void __cyg_profile_func_exit(void *this_fn, void *call_site) {
    // haven't started tracing yet.
    if(!can_print_p)
        return;
    assert(depth);

    can_print_p = 0;
    indent(depth*5); output("leaving %x\n", this_fn);
    can_print_p = 1;
    depth--;

}

unsigned ck_verbose_p = 0;
int total_bytes_allocated = 0;
int total_bytes_freed = 0;
// should hold all allocated blocks
static hdr_t *alloc_list; 
// free list 
static hdr_t *free_list; 
// returns pointer to the first allocated header block.
hdr_t *ck_first_alloc(void) {
    return alloc_list;
}

// returns pointer to the first free'd header block.
hdr_t *ck_first_free(void) {
    return free_list;
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

// are redzone bytes not overwritten? or free block's data being overwritten?
int mem_check(unsigned * nblks, hdr_t * h, int check_data) {
    int check = 0;
    for (int i = 0; i < REDZONE_NBYTES; i++) {
        char b1 = h->rz1[i];
        char b2 = *((char *)h + sizeof *h + h->nbytes_alloc + i); 
        int offset;
        if (b1 != REDZONE_VAL) {
            int offset = -(REDZONE_NBYTES - i) -2;
            ck_error(h, "%s block %u [%p] corrupted at offset %d\n",
                h->state == FREED ? "Freed" : "Allocated",
                    h->block_id, h, offset);
            check = 1;
        } else if (b2 != REDZONE_VAL) {
            int offset = h->nbytes_alloc + i;
            ck_error(h, "%s block %u [%p] corrupted at offset %d\n",
                h->state == FREED ? "Freed" : "Allocated",
                    h->block_id, h, offset);
            check = 1;
        }
    } 
    if (check_data == 1) { // if we are checking data bytes of freed blocks 
        for (char* i = (char *)h + sizeof *h; i < (char *)h + sizeof *h + h->nbytes_alloc; i++) { // for each byte
            if (*i != REDZONE_VAL) {
                check = 2;
                ck_error(h, "%s block %u [%p] corrupted at offset %d\n",
            h->state == FREED ? "Freed" : "Allocated",
                h->block_id, h, i-((char *)h + sizeof(hdr_t)));
            }   
        }
    }
    return check;
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
    
    // Before changin state, check redzone! 
    unsigned * rand_num = 0;
    int red_err = mem_check(rand_num, h, 0);
    if (red_err) {
        ck_panic(h,"corrupted block %d \n", h->block_id);
    }
    
    // change state to freed
    h->state = FREED;

    // Set the data portion of the allocation to the redzone value so you can check that it isn't modified after.
    memset((char *)h + sizeof *h, REDZONE_VAL, h->nbytes_alloc);

    // remove block from the allocated list.
    hdr_t *preh = NULL;
    for (hdr_t *curh = ck_first_alloc(); curh; curh = ck_next_hdr(curh)) {
        // output("inside for loop\n");
        if (curh == h) {
            if (curh == ck_first_alloc()) {
                alloc_list = curh->next;
            } else {
                preh->next = curh->next;
            }
            break;
        }
        preh = curh;
    }
    total_bytes_allocated -= h->nbytes_alloc;

    // kr_free(h);
    // Instead of calling kr_free, add to free_list
    if (total_bytes_freed == 0) { // if there is nth in free list 
        free_list = h;
    } else { // add to the end of free list 
        hdr_t *curh = free_list;
        hdr_t *preh = free_list;
        while (curh!=NULL) { 
            preh = curh;
            curh = curh->next; 
        }
        // last header of the allocated list should point to this header 
        preh->next = h;
    }
    // set next to NULL so that when we traverse the list, we know when to stop.
    h->next = NULL;
    total_bytes_freed += h->nbytes_alloc;
}

// interpose on kr_malloc allocations and
//  1. allocate enough space for a header and fill it in.
//  2. add the allocated block to  the allocated list.
void *(ckalloc)(uint32_t nbytes, src_loc_t l) {
    static unsigned block_id=1;
    // lab 13: incase user wants their own alloc
    // ckalloc_init
    // add second redzone. first redzone already included in header
    hdr_t *h = alloc_fn(nbytes + sizeof *h + REDZONE_NBYTES); 
     
    memset(h, 0, sizeof *h);
    h->nbytes_alloc = nbytes;
    h->state = ALLOCED;
    h->alloc_loc = l;
    h->block_id = block_id++;
    // Initialize first redzone
    memset(h->rz1, REDZONE_VAL, REDZONE_NBYTES);
    // Initialize second redzone. after data bytes
    memset((char *)h + sizeof *h + h->nbytes_alloc, REDZONE_VAL, REDZONE_NBYTES);

    // set addr;
    void *addr = 0;

    // todo("put on allocated list\n");
    if (total_bytes_allocated == 0) {
        alloc_list = h;
    } else {
        hdr_t *curh = alloc_list;
        hdr_t *preh = alloc_list;
        while (curh!=NULL) {
            preh = curh;
            curh = curh->next; 
        }
        // last header of the allocated list should point to this header 
        preh->next = h;
    }
    // increment nbytes_alloced
    total_bytes_allocated += nbytes;
    addr = ck_data_start(h);
    assert(ck_ptr_is_alloced(addr));
    if(ck_verbose_p)
        loc_debug(l, "successful alloc of %p\n", addr);
    return addr;
}

int check_list(unsigned * nblks, hdr_t * list, int num) {
    int cnt_errs = 0;
    for(hdr_t *h = list; h; h = h->next) {
        int check = mem_check(nblks, h, num);
        if (check) {// check = 1 if there is a problem 
            if (num == 1 && check == 2) { // if we are checking data bytes and there is a problem, print out the error message
                trace("	Wrote block after free!\n");
            }
            cnt_errs++; // everytime there is a problem, increment the error count
        }
        *nblks += 1; // number of blocks that we checked.
    }
    return cnt_errs;
}
// integrity check the allocated / freed blocks in the heap
int ck_heap_errors(void) {
    trace("going to check heap\n");

    unsigned nblks = 0;

    unsigned nerrors = check_list(&nblks, alloc_list,0)
                     + check_list(&nblks, free_list, 1);

    if(nerrors)
        trace("checked %d blocks, detected %d errors\n", nblks, nerrors);
    else
        trace("SUCCESS: checked %d blocks, detected no errors\n", nblks);
    return nerrors;
}

hdr_t *ck_get_containing_blk(void *addr) {
    uint32_t ad = (uint32_t) addr;
    for(hdr_t *h = ck_first_alloc(); h; h = ck_next_hdr(h)) {
        if (ad >= (uint32_t)&h[0] && ad <= ((uint32_t)&h[0] + h->nbytes_alloc + sizeof *h + REDZONE_NBYTES)) {
            return h; 
        }
    }
    for(hdr_t *h = ck_first_free(); h; h = ck_next_hdr(h)) {
        if (ad >= (uint32_t)&h[0] && ad <= ((uint32_t)&h[0] + h->nbytes_alloc + sizeof *h + REDZONE_NBYTES)) {
            return h; 
        }
    }
    return NULL;
}