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
    if (init_flag != 0x1) {
        if (sf_init() != 0) return NULL;
    }

    info("Allocating memory...");
    // Check if requested size is 0; return NULL if true
    if (size == 0) {
        debug("sf_malloc: request size is 0; returning NULL");
        return NULL;
    } else {
        debug("sf_malloc: request size is %ld", size);
    }

    // Convert the payload size to min required size
    size_t rsize = MIN_REQUIRED_BLOCKSIZE(size);

    // Find block of required size
    info("Searching free block of size %ld...", rsize);
    sf_block *bp;
    int index = get_free_list_index_for_size(rsize);
    // For loop to search in jth free lists for j from index to max
    for (int j = index; j < NUM_FREE_LISTS; j++) {
        // Search for free block in jth free list
        bp = find_in_free_list_i(j, rsize);

        if (bp == NULL) {
            // If free block not found in jth free list then continue
            if (j == NUM_FREE_LISTS - 1) {
                // If free block not found in last free list then expand heap
                info("No free blocks found...");
                sf_block *fbp = expand_heap();
                if (fbp == NULL) {
                    return NULL;
                }
                fbp = coalesce_block(fbp);
                add_block_to_free_list(fbp);
                j--;
            } else continue;
        } else {
            info("Free block of size %ld found...", BLOCKSIZE(bp));
            remove_block_from_free_list(bp);
            update_block_header(bp, bp->header | 0b10000);
            if (BLOCKSIZE(bp) - rsize >= MINBLOCKSIZE) {
                // If free block found can be broken without splinter
                sf_block *rbp = break_block(bp, rsize);
                // update_block_header(bp, bp->header | 0b10000);
                add_block_to_free_list(rbp);
            }
        }
    }
    info("Memory allocated");
    return bp->body.payload;
}

void sf_free(void *pp) {
    // Validate the pointer to be freed
    info("Validating the pointer to be freed...");
    if (validate_pointer(pp) != 0) {
        sf_errno = EINVAL;
        abort();
    }
    // Free the block by updating header & footer
    info("Freeing the memory allocated at %p", pp);
    sf_block *bp = (sf_block *)((char *) pp - MEMROWSIZE);
    update_block_header(bp, bp->header & ~ 0b10000);

    // Coalesce the block and add to free list
    bp = coalesce_block(bp);
    add_block_to_free_list(bp);
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    // Validate the pointer to be reallocated
    info("Validating the pointer to be reallocated...");
    if (validate_pointer(pp) != 0) {
        sf_errno = EINVAL;
        return NULL;
    }

    info("Reallocating the memory at %p...", pp);
    // Calculate block pointer and block sizes
    sf_block *bp = (sf_block *)((char *) pp - MEMROWSIZE);
    size_t blocksize = BLOCKSIZE(bp);
    size_t new_blocksize = MIN_REQUIRED_BLOCKSIZE(rsize);

    if (rsize == 0) {
        // If block is to be reallocated to size 0 then call sf_free and return null
        sf_free(pp);
        return NULL;
    } else if (new_blocksize == blocksize) {
        // If block is to be reallocated to same size then return pp
        return pp;
    } else if (new_blocksize > blocksize) {
        // If block is to be reallocated to larger size then call sf_malloc, copy contents and call sf_free
        void *npp = sf_malloc(rsize);
        memcpy(npp, pp, blocksize - MEMROWSIZE);
        sf_free(pp);
        return npp;
    } else {
        // If block is to be reallocated to smaller size then break block, coalesce remainder contents and add to free list
        sf_block *rbp = break_block(bp, new_blocksize);
        update_block_header(bp, bp->header | 0b10000);
        rbp = coalesce_block(rbp);
        add_block_to_free_list(rbp);
        return pp;
    }
}

void *sf_memalign(size_t size, size_t align) {
    info("Allocating memory of alignment %ld...", align);
    info("Validating the alignment arg...");
    // Validate if align is > 2^5
    if (align < 32) {
        sf_errno = EINVAL;
        return NULL;
    }
    // Validate if align is a power of 2
    if ((align & (align - 1)) != 0) {
        sf_errno = EINVAL;
        return NULL;
    }

    // Calculate min required block size from malloc
    sf_block *bp;
    size_t blocksize = MIN_REQUIRED_BLOCKSIZE(size);
    size_t larger_blocksize = blocksize + align - MINBLOCKSIZE;

    // Allocate memory for larger block
    info("Calling sf_malloc for larger block of size %ld...", larger_blocksize);
    void *pp = sf_malloc(larger_blocksize);
    if (pp == NULL) {
        return NULL;
    }

    // Split block in the beginning
    size_t precede_blocksize = ((size_t)pp % align) ? align - (size_t)pp % align : 0;
    sf_block *lbp = (sf_block *)((sf_header *)pp - 1);
    if (precede_blocksize > 0) {
        info("Breaking the block to size %ld...", precede_blocksize);
        bp = break_block(lbp, precede_blocksize);
        update_block_header(lbp, lbp->header & ~ 0b10000);
        update_block_header(bp, bp->header | 0b10000);
        lbp = coalesce_block(lbp);
        add_block_to_free_list(lbp);
    } else {
        bp = lbp;
    }

    // Split block in the beginning
    if (BLOCKSIZE(bp) - blocksize >= MINBLOCKSIZE) {
        info("Breaking the excess block to size %ld...", blocksize);
        sf_block *rbp = break_block(bp, blocksize);
        rbp = coalesce_block(rbp);
        add_block_to_free_list(rbp);
    }
    info("Memory allocated");
    return bp->body.payload;
}
