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
#include "initialise.h"

int sf_init() {
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

    // Heap alignment
    void *heap_start = sf_mem_start();
    if ((long int) heap_start % 16 != 0) return -1;
    if ((long int) heap_start % 32 == 0) offset = (3 * 8); else offset = (1 * 8);
    debug("mem_start : %p", heap_start);
    debug("mem_end   : %p", sf_mem_end());
    debug("offset    : %d", offset);

    // get new_memblock
    sf_mem_grow();
    debug("after memory grow:");
    debug("mem_start : %p", heap_start);
    debug("mem_end   : %p", sf_mem_end());

    // update prologue
    update_prologue();

    // update pointers by adding the block to freelist
    first_free_block(offset);

    // update epilogue
    update_epilogue(offset);
    
    debug("Initialization complete");
    return 0;
}

void *sf_malloc(size_t size) {
    // Initialize the sf_malloc
    if (global_ != 0x1) {
        int status = sf_init();
        if (status == -1) debug("error");
        void *p1 = sf_mem_start();
        return p1;
    }

    // Check if requested size is 0; return NUll if true
    if (size == 0) {
        debug("Request size is 0");
        return NULL;
    }

    // Convert the size to required min block size
    size_t required_size = min_blocksize(size);
    int index = get_free_list_index_for_size(required_size);

    void *bp;
    sf_block block;

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
            block = get_sf_block(bp);
            if (get_blocksize_from_header(block.header) - required_size >= ALIGNMENT) {
                // TODO: break the block
                *((long int* )bp) = *((long int*) bp) | 0x1000;
            }
        }
    }
    // sf_block block = get_sf_block(bp)
    return bp+8;
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
