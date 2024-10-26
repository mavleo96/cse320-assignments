#include "sfmm_utils.h"

/*
 * Helper function to get free list index for given size
 */
int get_free_list_index_for_size(size_t size) {
    // Check if size is aligned; return -1 if not
    if (size % MINBLOCKSIZE != 0) return -1;
    // Convert size to multiples of minimum size
    size = size / MINBLOCKSIZE;

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

/*
 * Helper function to find a block of given size in free list of given index
 */
sf_block *find_in_free_list_i(int index, size_t size) {
    // Sentinel node of free list
    sf_block *list_head = &sf_free_list_heads[index];

    // Traverse through free list
    sf_block *bp = &sf_free_list_heads[index];
    // Continue checking for free block until next block is sentinel node
    while (bp->body.links.next != list_head) {
        bp = bp->body.links.next;
        if (BLOCKSIZE(bp) >= size) {
            return bp;
        }
    }
    // Return NULL if no free block found
    return NULL;
}

/*
 * Helper function to remove a block from free list
 */
void remove_block_from_free_list(sf_block *bp) {
    debug("removing %p from free list...", bp);
    // Validate block pointer 
    if (bp == NULL) {
        error("NULL pointer passed!");
        return;
    }
    if ((bp->body.links.next == NULL) || (bp->body.links.prev == NULL)) {
        error("pointer with NULL links passed!");
        return;
    }
    // Update the links in next and prev block
    bp->body.links.next->body.links.prev = bp->body.links.prev;
    bp->body.links.prev->body.links.next = bp->body.links.next;
    return;
}

/*
 * Helper function to add a block to appropriate free list
 */
void add_block_to_free_list(sf_block *bp) {
    debug("adding %p to free list...", bp);
    // Validate block pointer 
    if (bp == NULL) {
        error("NULL pointer passed!");
        return;
    }
    if (ALLOCATED_BIT(bp)) {
        error("allocated block passed!");
    }

    // Check if the free block passed is wilderness block
    int index;
    if (NEXT_BLOCK_POINTER(bp) == EPILOGUE_POINTER) {
        // Add to last free list if the block is wilderness block
        index = NUM_FREE_LISTS - 1;
    } else {
        index = get_free_list_index_for_size(BLOCKSIZE(bp));
    }

    // Add the free block to the list by updating links
    sf_block *list_head = &sf_free_list_heads[index];
    bp->body.links.prev = list_head;
    bp->body.links.next = list_head->body.links.next;

    // Update the links in next and prev block
    bp->body.links.next->body.links.prev = bp;
    bp->body.links.prev->body.links.next = bp;
    return;
}