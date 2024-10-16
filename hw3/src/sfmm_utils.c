#include "debug.h"
#include "sfmm.h"
#include "sfmm_utils.h"

/*
 * Helper function to get index of sf_free_list_heads to access for given size
 * TODO: check if this can be made inline to avoid additional function call overhead
 */
int get_free_list_index_for_size(size_t size) {
    // Check if size is aligned; return -1 if not
    if (size % ALIGNMENT != 0) return -1;
    // Convert size to multiples of alignments
    size = size / ALIGNMENT;

    // Return index by comparing size against fibonacci series
    if (size > 34)      return 8;
    else if (size > 21) return 7;
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
 * Helper function to get block size from payload size
 * TODO: check if this can be made inline to avoid additional function call overhead
 */
int min_blocksize(size_t size) {
    // Add size of header to payload size 
    size += HEADER_SIZE;
    // Add padding to align block boundaries
    size += (size % ALIGNMENT != 0) ? (ALIGNMENT - size % ALIGNMENT) : 0;

    return size;
}

/*
 * Helper function to read block size from a given header
 */
size_t get_blocksize_from_header(sf_header header) {
    // TODO: add comments and error handling
    sf_header mask = ~ 0b11111;
    size_t size = header & mask;

    return size;
}

/*
 * Helper function to find a free block for a given in the specified index of free_lists
 */
sf_block *find_in_free_list_i(int index, size_t size) {
    debug("Searching for a block of size %ld in free list %d", size, index);
    sf_block *list_head = &sf_free_list_heads[index];
    sf_block *block = &sf_free_list_heads[index];

    while (1) {
        if (block->body.links.next == list_head) {
            debug("Returning null since no block found in free list %d", index);
            return NULL;
        }
        if (get_blocksize_from_header(block->header) >= size) {
            (block->body.links.next)->body.links.prev = block->body.links.prev;
            (block->body.links.prev)->body.links.next = block->body.links.next;
            break;
        } else {
            block = block->body.links.next;
        }
    }
    // TODO: block->body.payload = (char *) block + HEADER_SIZE;
    // TODO: Error handling to be done
    return block;
}