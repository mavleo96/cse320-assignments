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
