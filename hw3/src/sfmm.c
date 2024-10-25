/**
 * Do not submit your assignment with a main function in this file.
 * If you submit with a main function in this file, you will get a zero.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "debug.h"
#include "sfmm.h"
#include "sfmm_utils.h"

void *sf_malloc(size_t size) {
    // Initialize the sf_malloc
    if (global_ != 0x1) sf_init();

    // Check if requested size is 0; return NUll if true
    if (size == 0) {
        info("Request size is 0");
        return NULL;
    }

    // Convert the payload size to min required size and get 
    size_t required_size = min_required_blocksize(size);
    int index = get_free_list_index_for_size(required_size);

    // Find block of required_size
    sf_block *bp;
    for (int j = index; j < NUM_FREE_LISTS; j++) {
        bp = find_in_free_list_i(j, required_size);
        if (bp == NULL) {
            if (j == NUM_FREE_LISTS - 1) {
                sf_block *fbp = expand_heap();
                fbp = coalesce_block(fbp);
                add_block_to_free_list(fbp, 1);
                j--;
            } else continue;
        } else {
            remove_block_from_free_list(bp);
            if (get_blocksize(bp) - required_size >= ALIGNMENT) {
                sf_block *rbp = break_block(bp, required_size);
                add_block_to_free_list(rbp, (j == NUM_FREE_LISTS - 1) ? 1 : 0);
            }
        }
    }
    info("Memory allocated");
    return bp->body.payload;
}

void sf_free(void *pp) {
    info("Validating the pointer to be freed...");
    validate_pointer(pp, 0);

    info("Freeing the memory allocated at %p", pp);
    sf_block *bp = (sf_block *)((char *) pp - MEMROWSIZE);
    bp->header &= ~ 0b10000;
    *get_footer(bp) = bp->header;

    sf_block *nbp = get_next_block(bp);
    nbp->header &= ~ 0b1000;
    if (!get_allocated_bit(nbp)) *get_footer(nbp) = nbp->header;

    bp = coalesce_block(bp);
    add_block_to_free_list(bp, 0);

    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    info("Validating the pointer to be reallocated...");
    validate_pointer(pp, 0);

    info("Freeing the memory allocated at %p", pp);
    sf_block *bp = (sf_block *)((char *) pp - MEMROWSIZE);
    size_t block_size = get_blocksize(bp);
    size_t new_block_size = min_required_blocksize(rsize);

    if (rsize == 0) {
        sf_free(pp);
        return NULL;
    } else if (new_block_size == block_size) {
        return pp;
    } else if (new_block_size > block_size) {
        void *npp = sf_malloc(rsize);
        memcpy(npp, pp, block_size - MEMROWSIZE);
        sf_free(pp);
        return npp;
    } else {
        sf_block *rbp = break_block(bp, new_block_size);
        rbp = coalesce_block(rbp);
        add_block_to_free_list(rbp, 0);
        return pp;
    }
}

void *sf_memalign(size_t size, size_t align) {
    // To be implemented
    abort();
}
