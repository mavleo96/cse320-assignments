#ifndef SFMM_UTILS_H
#define SFMM_UTILS_H

#include <stddef.h>
#include <errno.h>

#include "debug.h"
#include "sfmm.h"

int init_flag;
#define ALIGNMENT 32
#define MINBLOCKSIZE 32
#define MEMROWSIZE 8
int offset;

// Macros to parse information from block header
#define BLOCKSIZE(bp)          ((bp)->header & ~0b11111)    // Mask 5 LSB to get the block size
#define ALLOCATED_BIT(bp)      (((bp)->header & 0b10000) >> 4)
#define PREV_ALLOCATED_BIT(bp) (((bp)->header & 0b01000) >> 3)
// Macros to get useful pointers
#define NEXT_BLOCK_POINTER(bp) ((sf_block *)((char *)(bp) + BLOCKSIZE(bp)))
#define FOOTER_POINTER(bp)     ((sf_footer *)((char *)(bp) + BLOCKSIZE(bp) - MEMROWSIZE))
#define PROLOGUE_POINTER       ((sf_block *)((char *)sf_mem_start() + offset))
#define EPILOGUE_POINTER       ((sf_block *)((char *)sf_mem_end() - MEMROWSIZE))
// Macro to calculate minimum required block size from payload size
#define MIN_REQUIRED_BLOCKSIZE(size) \
    (((size) + MEMROWSIZE + ((ALIGNMENT - ((size) + MEMROWSIZE) % ALIGNMENT) % ALIGNMENT)))

// Auxillary Functions
int validate_pointer(void *pp);
 void update_block_header(sf_block *bp, sf_header header);

// Block Functions
sf_block *break_block(sf_block *bp, size_t required_size);
sf_block *expand_heap();
sf_block *coalesce_block(sf_block *bp);

// Free List Functions
int get_free_list_index_for_size(size_t size);
sf_block *find_in_free_list_i(int index, size_t size);
void remove_block_from_free_list(sf_block *bp);
void add_block_to_free_list(sf_block *bp);

// Initialize Functions
void update_prologue();
void update_epilogue();
void add_wilderness_block();
int sf_init();

#endif
