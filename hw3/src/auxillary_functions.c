#include "sfmm_utils.h"

/*
 * Helper function to validate pointer for sf_free and sf_realloc function
 */
void validate_pointer(void *pp, int mode) {
    if ((pp == NULL) || ((long int) pp % ALIGNMENT != 0)) abort();

    sf_block *bp = (sf_block *)((char *) pp - MEMROWSIZE);

    if ((bp <= PROLOGUE_POINTER) || (bp >= EPILOGUE_POINTER)) abort();
    if ((sf_block *) FOOTER_POINTER(bp) >= (sf_block *) sf_mem_end()) abort();

    size_t block_size = BLOCKSIZE(bp);

    if (mode) {
        if ((block_size < 32) || (block_size % 32 != 0)) abort();
        if (!ALLOCATED_BIT(bp)) abort();
    } else {
        if (block_size % 32 != 0) abort();
    }
    if ((!PREV_ALLOCATED_BIT(bp)) && (ALLOCATED_BIT((sf_block *)((sf_header *) bp - 1)))) abort();
    
    return;
}