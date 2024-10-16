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

void sf_init() {
    // TODO: error handling needed here
    debug("Initializing sf_malloc...");
    debug("Initializing sf_free_list_heads...");
    for (int i = 0; i < NUM_FREE_LISTS; i++) {
        sf_block new_block = {
            .header = 0,
            .body.links = {
                .next = &sf_free_list_heads[i],
                .prev = &sf_free_list_heads[i],
            },
        };
        sf_free_list_heads[i] = new_block;
    }
    debug("Setting global_...");
    global_ = 0x1;

    // get new_memblock
    // create header footer
    // update pointers by adding the block to freelist
    
    debug("Initialization complete");
    return;
}

void *sf_malloc(size_t size) {
    // Initialize the sf_malloc
    if (global_ != 0x1)
        sf_init();

    // Check if requested size is 0; return NUll if true
    if (size == 0) {
        debug("Request size is 0");
        return NULL;
    }

    // Convert the size to required min block size
    size_t required_size = min_blocksize(size);
    int index = get_free_list_index_for_size(required_size);

    sf_block *bp;

    for (int j = index; j < NUM_FREE_LISTS; j++) {
        bp = find_in_free_list_i(j, required_size);
        if (bp == NULL) {
            if (j == NUM_FREE_LISTS - 1) {
                // TODO: expand & coalesce heap
                j--;
            } else {
                continue;
            }
        } else {
            if (get_blocksize_from_header(bp->header) - required_size >= ALIGNMENT) {
                // TODO: break the block
            }
        }
    }
    return bp->body.payload;
}

void sf_free(void *pp) {
    // To be implemented
    return;
}

void *sf_realloc(void *pp, size_t rsize) {
    // To be implemented
    abort();
}

void *sf_memalign(size_t size, size_t align) {
    // To be implemented
    abort();
}
