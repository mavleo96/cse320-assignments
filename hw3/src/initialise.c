#include "sfmm_utils.h"

/*
 * Helper function to intialize prologue
 */
void update_prologue() {
    info("Updating prologue...");
    sf_block *pp = (sf_block *)((char *) sf_mem_start() + offset);
    // Prologue header & footer
    pp->header = 0x20 + 0b10000;
    *get_footer(pp) = pp->header;
    debug("prologue: %p", pp);
}

/*
 * Helper function to initialize epilogue
 */
void update_epilogue() {
    info("Updating epilogue...");
    sf_block *ep = (sf_block *)((char *) sf_mem_end() - MEMROWSIZE);
    // Epilogue header & footer
    ep->header = 0x0 + 0b10000;
    debug("epilogue: %p", ep);
}

/*
 * Helper function to initalise free block
 */
void add_wilderness_block() {
    // TODO: make this function better
    info("Adding Wilderness block...");
    sf_block *bp = (sf_block *)((char *) sf_mem_start() + offset + 4 * MEMROWSIZE);
    size_t block_size = (size_t) sf_mem_end() - (size_t) bp - MEMROWSIZE;

    bp->header = block_size + 0b1000;
    *get_footer(bp) = bp->header;
    debug("wilderness block header %ld %ld", bp->header, block_size);

    // pointers
    bp->body.links.next = &sf_free_list_heads[8];
    bp->body.links.prev = &sf_free_list_heads[8];
    
    sf_free_list_heads[8].body.links.next = bp;
    sf_free_list_heads[8].body.links.prev = bp;
}

void sf_init() {
    info("Initializing sf_malloc...");

    // Initialize sf_free_list_heads with sentinel nodes
    info("Initializing sf_free_list_heads...");
    for (int i = 0; i < NUM_FREE_LISTS; i++) {
        sf_free_list_heads[i].body.links.next = &sf_free_list_heads[i];
        sf_free_list_heads[i].body.links.prev = &sf_free_list_heads[i];
    }

    // Check heap pointers
    info("Getting heap start pointer...");
    void *heap_start = sf_mem_start();
    if ((long int) heap_start % 16 != 0) error("Heap start is not 16 byte aligned!");
    if ((long int) heap_start % 32 == 0) offset = (3 * MEMROWSIZE); else offset = (1 * MEMROWSIZE);
    debug("heap start, offset: %p, %d", heap_start, offset);

    // Expand heap
    info("Expanding heap...");
    sf_mem_grow();
    debug("mem_end: %p", sf_mem_end());

    update_prologue();
    add_wilderness_block();
    update_epilogue();
    
    info("Setting global_...");
    global_ = 0x1;
    
    info("Initialization complete");
    return;
}