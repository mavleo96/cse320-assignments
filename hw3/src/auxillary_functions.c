#include "sfmm_utils.h"

/*
 * Helper function to validate pointer for sf_free and sf_realloc function
 */
int validate_pointer(void *pp) {
    // Check is pointer is NULL or not aligned
    if (pp == NULL) {
        error("NULL pointer passed!");
        return -1;
    }
    if ((long int) pp % ALIGNMENT != 0) {
        error("pointer passed is not 32 byte aligned!");
        return -1;
    }

    // Check if block pointer is within heap
    sf_block *bp = (sf_block *)((char *) pp - MEMROWSIZE);
    if (bp <= PROLOGUE_POINTER) {
        error("prologue pointer passed!");
        return -1;
    }
    if (bp >= EPILOGUE_POINTER) {
        error("epilogue pointer passed!");
        return -1;
    }
    if ((sf_block *) FOOTER_POINTER(bp) >= (sf_block *) sf_mem_end()) {
        error("pointer passed is not within heap!");
        return -1;
    }

    // Check if block header is valid
    if (!ALLOCATED_BIT(bp)) {
        error("pointer to free block passed!");
        return -1;
    }
    if ((!PREV_ALLOCATED_BIT(bp)) && (ALLOCATED_BIT((sf_block *)((sf_header *) bp - 1)))) {
        error("pointer to block with corrupted header passed!");
        return -1;
    }
    if ((BLOCKSIZE(bp) < 32) || (BLOCKSIZE(bp) % 32 != 0)) {
        error("pointer to block with invalid size passed!");
        return -1;
    }
    return 0;
}

/*
 * Helper function to update block header and footers
 */
 void update_block_header(sf_block *bp, sf_header header) {
    // Update header and footer of block
    bp->header = header;
    if (!ALLOCATED_BIT(bp)) *FOOTER_POINTER(bp) = bp->header;

    // Update prev_allocated_bit in next block
    sf_block *nbp = NEXT_BLOCK_POINTER(bp);
    if (ALLOCATED_BIT(bp)) nbp->header |= 0b01000;
    else nbp->header &= ~ 0b01000;
    if (!ALLOCATED_BIT(nbp)) *FOOTER_POINTER(nbp) = nbp->header;
 }