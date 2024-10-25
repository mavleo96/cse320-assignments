#include "sfmm_utils.h"

/*
 * Helper function to intialize/update prologue
 */
void update_prologue() {
    info("Updating prologue...");
    PROLOGUE_POINTER->header = (sf_header)(0x20 + 0b10000);  // (blocksize, allocated, prev_allocated) -> (32, 1, 0)
}

/*
 * Helper function to initialize/update epilogue
 * Called inside sf_init and expand_heap with the assumption that preceding block is free 
 */
void update_epilogue() {
    info("Updating epilogue...");
    EPILOGUE_POINTER->header = (sf_header)(0x0 + 0b10000);   // (blocksize, allocated, prev_allocated) -> (0, 1, 0)
}

/*
 * Helper function to initalize wilderness block and add to free lists
 */
void add_wilderness_block() {
    info("Adding Wilderness block...");
    // Calculate wilderness block pointer and blocksize
    sf_block *bp = NEXT_BLOCK_POINTER(PROLOGUE_POINTER);
    size_t blocksize = (size_t) EPILOGUE_POINTER - (size_t) bp;

    // Update block header
    // Previous block is prologue block which is allocated by definition
    bp->header = (sf_header)(blocksize + 0b01000);  // (blocksize, allocated, prev_allocated) -> (blocksize, 0, 1)
    *FOOTER_POINTER(bp) = bp->header;

    // Add wilderness block to free list
    add_block_to_free_list(bp, 1);
    // TODO: remove below commented code after testing
    // bp->body.links.next = &sf_free_list_heads[8];
    // bp->body.links.prev = &sf_free_list_heads[8];
    
    // sf_free_list_heads[8].body.links.next = bp;
    // sf_free_list_heads[8].body.links.prev = bp;
}

/*
 * Function to initialize the malloc by initializing the free lists, expanding heap & setting prologue and epilogue
 */
void sf_init() {
    info("Initializing sf_malloc...");

    // Initialize sf_free_list_heads with sentinel nodes
    info("Initializing sf_free_list_heads...");
    for (int i = 0; i < NUM_FREE_LISTS; i++) {
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }

    // Assert that heap is aligned
    info("Getting heap start pointer...");
    void *heap_start = sf_mem_start();
    if ((long int) heap_start % 16 != 0) error("Heap start is not 16 byte aligned!");
    if ((long int) heap_start % 32 == 0) offset = (3 * MEMROWSIZE); else offset = (1 * MEMROWSIZE);

    // Expand heap
    info("Expanding heap...");
    sf_mem_grow();
    debug("(mem_start, mem_end, offset) -> (%p, %p, %d)", heap_start, sf_mem_end(), offset);

    // Initialize the heap
    update_prologue();
    add_wilderness_block();
    update_epilogue();
    
    // Set the init_flag to 1
    info("Setting init_flag...");
    init_flag = 0x1;
    
    info("Initialization complete");
    return;
}