#include "sfmm_utils.h"

sf_block *break_block(sf_block *bp, size_t required_size) {

    size_t block_size = get_blocksize(bp);

    sf_block *rbp = (sf_block *)((char *) bp + required_size);

    bp->header = required_size + (bp->header & 0b11111);
    rbp->header = (sf_header) (block_size - required_size + 0b01000);
    *get_footer(rbp) = rbp->header;

    *(sf_header *)((char *) bp + block_size) &= ~ 0b1000;

    return rbp;
}

sf_block *expand_heap() {
    sf_block *bp = (sf_block *)((char *) sf_mem_end() - MEMROWSIZE);
    info("Expanding heap...");
    sf_mem_grow();
    update_epilogue();
    bp->header += PAGE_SZ;
    *get_footer(bp) = bp->header;
    
    return bp;
}

sf_block *coalesce_block(sf_block *bp) {
    int prev_allocated = get_prev_allocated_bit(bp);
    int next_allocated = get_allocated_bit(get_next_block(bp));
    if (prev_allocated & next_allocated) return bp;
    if (!next_allocated) {
        size_t next_block_size = get_blocksize(get_next_block(bp));
        bp->header += next_block_size;
        remove_block_from_free_list(get_next_block(bp));
        bp->header &= ~ 0b10000;
    }
    if (!prev_allocated) {
        size_t prev_block_size = get_blocksize((sf_block *)((sf_header *) bp - 1));
        size_t block_size = get_blocksize(bp);
        bp = (sf_block *)((char *) bp - prev_block_size);
        bp->header += block_size;
        remove_block_from_free_list(bp);
        // bp->header &= ~ 0b10000; 
    }
    *get_footer(bp) = bp->header;
    return bp;
}