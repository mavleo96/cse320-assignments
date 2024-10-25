#include "sfmm_utils.h"

int get_free_list_index_for_size(size_t size) {
    // Check if size is aligned; return -1 if not
    if (size % ALIGNMENT != 0) return -1;
    // Convert size to multiples of alignments
    size = size / ALIGNMENT;

    // Return index by comparing size against fibonacci series
    if (size > 21) return 7;
    else if (size > 13) return 6;
    else if (size > 8)  return 5;
    else if (size > 5)  return 4;
    else if (size > 3)  return 3;
    else if (size == 3) return 2;
    else if (size == 2) return 1;
    else if (size == 1) return 0;
    else                return -1;  // TODO: this means size zero; check if -1 needs to be returned or NULL handling needed
}

sf_block *find_in_free_list_i(int index, size_t size) {
    sf_block *list_head = &sf_free_list_heads[index];
    sf_block *bp = &sf_free_list_heads[index];
    
    while (bp->body.links.next != list_head) {
        bp = bp->body.links.next;
        if (BLOCKSIZE(bp) >= size) {
            return bp;
        }
    }
    return NULL;
}


void remove_block_from_free_list(sf_block *bp) {
    size_t block_size = BLOCKSIZE(bp);
    // Allocated bit change
    bp->header |= 0b10000;

    // Prev allocated bit change
    // TODO: what if this is a free block?
    *(sf_header *)((char *) bp + block_size) |= 0b1000;

    // TODO: next and prev should not be NULL here
    bp->body.links.next->body.links.prev = bp->body.links.prev;
    bp->body.links.prev->body.links.next = bp->body.links.next;
    return;
}

void add_block_to_free_list(sf_block *bp, int wilderness_signal) {
    size_t block_size = BLOCKSIZE(bp);
    int index = wilderness_signal ? NUM_FREE_LISTS - 1 : get_free_list_index_for_size(block_size);
    sf_block *list_head = &sf_free_list_heads[index];

    bp->body.links.prev = list_head;
    bp->body.links.next = list_head->body.links.next;

    bp->body.links.next->body.links.prev = bp;
    bp->body.links.prev->body.links.next = bp;

    return;
}