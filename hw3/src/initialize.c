#include "sfmm_utils.h"

/*
 * Helper function to intialize/update prologue
 */
void update_prologue() {
    debug("updating prologue...");
    PROLOGUE_POINTER->header = (sf_header)(0x20 + 0b10000);  // (blocksize, allocated, prev_allocated) -> (32, 1, 0)
}

/*
 * Helper function to initialize/update epilogue
 * Called inside sf_init and expand_heap with the assumption that preceding block is free 
 */
void update_epilogue() {
    debug("updating epilogue...");
    EPILOGUE_POINTER->header = (sf_header)(0x0 + 0b10000);   // (blocksize, allocated, prev_allocated) -> (0, 1, 0)
}

/*
 * Helper function to initalize wilderness block and add to free lists
 */
void add_wilderness_block() {
    debug("adding wilderness block...");
    // Calculate wilderness block pointer and blocksize
    sf_block *bp = NEXT_BLOCK_POINTER(PROLOGUE_POINTER);
    size_t blocksize = (size_t) EPILOGUE_POINTER - (size_t) bp;

    // Update block header
    // Previous block is prologue block which is allocated by definition
    update_block_header(bp, (sf_header)(blocksize + 0b01000));  // (blocksize, allocated, prev_allocated) -> (blocksize, 0, 1)
    // TODO: Interesting bug; above function doesn't modify footer in this place
    *FOOTER_POINTER(bp) = bp->header;

    // Add wilderness block to free list
    add_block_to_free_list(bp);
}

/*
 * Function to initialize the malloc by initializing the free lists, expanding heap & setting prologue and epilogue
 */
int sf_init() {
    info("initializing heap...");

    // Initialize sf_free_list_heads with sentinel nodes
    debug("initializing sf_free_list_heads...");
    for (int i = 0; i < NUM_FREE_LISTS; i++) {
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }

    // Assert that heap is aligned
    debug("getting heap start pointer...");
    void *heap_start = sf_mem_start();
    if ((long int) heap_start % 16 != 0) {
        error("heap start is not 16 byte aligned!");
        return -1;
    }
    if ((long int) heap_start % 32 == 0) offset = (3 * MEMROWSIZE); else offset = (1 * MEMROWSIZE);

    // Expand heap
    debug("expanding heap...");
    if (sf_mem_grow() == NULL) {
        sf_errno = ENOMEM;
        error("not enough memory!");
        return -1;
    }
    debug("(mem_start, mem_end, offset) -> (%p, %p, %d)", heap_start, sf_mem_end(), offset);

    // Initialize the heap
    update_prologue();
    add_wilderness_block();
    update_epilogue();
    
    // Set the init_flag to 1
    debug("setting init_flag...");
    init_flag = 0x1;
    
    info("initialization complete");
    return 0;
}