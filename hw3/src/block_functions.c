#include "sfmm_utils.h"

/*
 * Helper function to break a block and return pointer to remainder block
 */
sf_block *break_block(sf_block *bp, size_t rsize) {
    info("Breaking block...");
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
    
    // Update header and footer of block
    bp->header = (bp->header & 0b11111) + (sf_header)rsize;  // (blocksize, allocated, prev_allocated) -> (rsize, 0/1, 0/1)
    if (!ALLOCATED_BIT(bp)) *FOOTER_POINTER(bp) = bp->header;

    // Update header and footer of remainder block
    // TODO: check if previous block is always allocated
    rbp->header = (sf_header)(blocksize - rsize + 0b01000);  // (blocksize, allocated, prev_allocated) -> (blocksize - rsize, 0, 1)
    *FOOTER_POINTER(rbp) = rbp->header;

    // Update prev_allocated_bit in next block
    sf_block *nbp = NEXT_BLOCK_POINTER(rbp);
    nbp->header &= ~ 0b01000;
    if (!ALLOCATED_BIT(nbp)) *FOOTER_POINTER(nbp) = nbp->header;
    return rbp;
}

/*
 * Helper function to expand heap and return the free block created
 */
sf_block *expand_heap() {
    // Save the epilogue pointer which will be header to free block and heap expand
    sf_block *bp = EPILOGUE_POINTER;
 
    // Expand heap and update epilogue
    info("Expanding heap...");
    sf_mem_grow();
    update_epilogue();

    // Update header and footer of free block and return pointer
    bp->header += PAGE_SZ;
    *FOOTER_POINTER(bp) = bp->header;
    return bp;
}

/*
 * Helper function to coalesce block return the free block created
 */
sf_block *coalesce_block(sf_block *bp) {
    info("Coalescing the block...");
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
        bp->header += next_blocksize;
        bp->header &= ~ 0b10000;
    }

    // Coalesce with previous block
    int prev_allocated = PREV_ALLOCATED_BIT(bp);
    if (!prev_allocated) {
        size_t prev_blocksize = BLOCKSIZE((sf_block *)((sf_header *) bp - 1));
        size_t blocksize = BLOCKSIZE(bp);
        bp = (sf_block *)((char *) bp - prev_blocksize);
        remove_block_from_free_list(bp);
        bp->header += blocksize;
        bp->header &= ~ 0b10000;
    }
    *FOOTER_POINTER(bp) = bp->header;
    return bp;
}