#include "sfmm_utils.h"

/*
 * Helper function to read block size from a given header
 */
size_t get_blocksize(sf_block *bp) {
    return bp->header & ~ 0b11111;      // Mask 5 LSB
}

/*
 * Helper function to get block allocated bit
 */
int get_allocated_bit(sf_block *bp) {
    return (bp->header & 0b10000) >> 4;
}

/*
 * Helper function to get previous block allocated bit
 */
int get_prev_allocated_bit(sf_block *bp) {
    return (bp->header & 0b1000) >> 3;
}

/*
 * Helper function to get footer pointer of a block
 */
sf_footer *get_footer(sf_block *bp) {
    size_t block_size = get_blocksize(bp);
    return (sf_footer *)((char *) bp + block_size - MEMROWSIZE);
}

/*
 * Helper function to get previous block allocated bit
 */
sf_block *get_next_block(sf_block *bp) {
    return (sf_block *)((char *) bp + get_blocksize(bp));
}

/*
 * Helper function to get block size from payload size
 * TODO: check if this can be made inline to avoid additional function call overhead
 */
int min_required_blocksize(size_t size) {
    size += MEMROWSIZE;                                                         // Add size of header to payload size
    size += (size % ALIGNMENT != 0) ? (ALIGNMENT - size % ALIGNMENT) : 0;       // Add padding to align block boundaries
    return size;
}
