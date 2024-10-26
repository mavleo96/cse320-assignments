#include "sfmm_utils.h"

/*
 * Helper function to break a block and return pointer to remainder block
 */
sf_block *break_block(sf_block *bp, size_t rsize) {
    info("breaking block %p to size %ld...", bp, rsize);
    // Validate block pointer 
    if (bp == NULL) {
        error("NULL pointer passed!");
        return NULL;
    }
    if (BLOCKSIZE(bp) < rsize) {
        error("blocksize is smaller than rsize arg passed!");
        return NULL;
    }

    // Calculate pointer of remainder block and blocksize
    size_t blocksize = BLOCKSIZE(bp);
    sf_block *rbp = (sf_block *)((char *)bp + (sf_header)rsize);
    
    // Update header and footer of new blocks
    update_block_header(rbp, (sf_header)(blocksize - rsize + 0b00000));  // (blocksize, allocated, prev_allocated) -> (blocksize - rsize, 0, 0/1)
    update_block_header(bp, (bp->header & 0b11111) + (sf_header)rsize);  // (blocksize, allocated, prev_allocated) -> (rsize, 0/1, 0/1)

    return rbp;
}

/*
 * Helper function to expand heap and return the free block created
 */
sf_block *expand_heap() {
    // Save the epilogue pointer which will be header to free block and heap expand
    sf_block *bp = EPILOGUE_POINTER;
 
    // Expand heap and update epilogue
    debug("expanding heap...");
    if (sf_mem_grow() == NULL) {
        sf_errno = ENOMEM;
        error("not enough memory!");
        return NULL;
    }
    update_epilogue();

    // Update header and footer of free block and return pointer
    update_block_header(bp, (bp->header & ~ 0b10000) + PAGE_SZ);
    return bp;
}

/*
 * Helper function to coalesce block return the free block created
 */
sf_block *coalesce_block(sf_block *bp) {
    debug("coalescing the block...");
    // Validate block pointer 
    if (bp == NULL) {
        error("NULL pointer passed!");
        return NULL;
    }

    // Coalesce with next block
    int next_allocated = ALLOCATED_BIT(NEXT_BLOCK_POINTER(bp));
    if (!next_allocated) {
        size_t next_blocksize = BLOCKSIZE(NEXT_BLOCK_POINTER(bp));
        remove_block_from_free_list(NEXT_BLOCK_POINTER(bp));
        update_block_header(bp, bp->header + next_blocksize);
    }

    // Coalesce with previous block
    int prev_allocated = PREV_ALLOCATED_BIT(bp);
    if (!prev_allocated) {
        size_t prev_blocksize = BLOCKSIZE((sf_block *)((sf_header *) bp - 1));
        size_t blocksize = BLOCKSIZE(bp);
        bp = (sf_block *)((char *) bp - prev_blocksize);
        remove_block_from_free_list(bp);
        update_block_header(bp, bp->header + blocksize);
    }
    // Mark coalesced block as free
    update_block_header(bp, (bp->header & ~ 0b10000));
    return bp;
}