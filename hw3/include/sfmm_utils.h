#ifndef SFMM_UTILS_H
#define SFMM_UTILS_H

#include <stddef.h>

#include "debug.h"
#include "sfmm.h"

int global_;
#define ALIGNMENT 32
#define MEMROWSIZE 8
int offset;

// Auxillary Functions
size_t get_blocksize(sf_block *bp);
int get_allocated_bit(sf_block *bp);
int get_prev_allocated_bit(sf_block *bp);
sf_block *get_next_block(sf_block *bp);
int min_required_blocksize(size_t payload_size);
sf_footer *get_footer(sf_block *bp);

// Block Functions
sf_block *break_block(sf_block *bp, size_t required_size);
sf_block *expand_heap();
sf_block *coalesce_block(sf_block *bp);

// Free List Functions
int get_free_list_index_for_size(size_t size);
sf_block *find_in_free_list_i(int index, size_t size);
void remove_block_from_free_list(sf_block *bp);
void add_block_to_free_list(sf_block *bp, int wilderness_signal);

// Initialise Functions
void update_prologue();
void update_epilogue();
void add_wilderness_block();

#endif
