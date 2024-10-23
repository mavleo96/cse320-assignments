#include "debug.h"
#include "sfmm.h"
#include "initialise.h"

/*
 * Helper function to intialise prologue
 */
void update_prologue() {
    // TODO: make this function better
    debug("updating prologue");
    long int *heap_start = (long int *) sf_mem_start();
    // Prologue header
    *(heap_start + offset / 8) = 0x20 + 0x10;
}

/*
 * Helper function to initialise epilogue
 */
void update_epilogue() {
    // TODO: make this function better
    debug("updating epilogue");
    long int *heap_end = (long int *) sf_mem_end();

    // Prologue header
    *(heap_end - 1) = 0x0 + 0b10000;
}

/*
 * Helper function to initalise free block
 */
void first_free_block() {
    // TODO: make this function better
    debug("updating first free block");
    long int *block_start = ((long int *) sf_mem_start()) + offset / 8 + 4;
    long int *block_end = ((long int *) sf_mem_end()) - 1;
    size_t block_size = (size_t) block_end - (size_t) block_start;
    debug("%p", block_start);
    debug("%p", block_end);

    debug("%ld", block_size);
    debug("%ld", block_size % 32);
    
    // Header * footer
    *block_start = block_size + 0b1000;
    *(block_end - 1) = block_size + 0b1000;
    
    // pointers
    sf_free_list_heads[8].body.links.next = (struct sf_block *) block_start;
    sf_free_list_heads[8].body.links.prev = (struct sf_block *) block_start;

    *(block_start + 1) = (long int) &sf_free_list_heads[8];
    *(block_start + 2) = (long int) &sf_free_list_heads[8];
}
