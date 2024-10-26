#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#include "sfmm_utils.h"
#define TEST_TIMEOUT 15

/*
 * Assert the total number of free blocks of a specified size.
 * If size == 0, then assert the total number of all free blocks.
 */
void assert_free_block_count(size_t size, int count) {
    int cnt = 0;
    for(int i = 0; i < NUM_FREE_LISTS; i++) {
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	while(bp != &sf_free_list_heads[i]) {
	    if(size == 0 || size == (bp->header & ~0x1f))
		cnt++;
	    bp = bp->body.links.next;
	}
    }
    if(size == 0) {
	cr_assert_eq(cnt, count, "Wrong number of free blocks (exp=%d, found=%d)",
		     count, cnt);
    } else {
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %ld (exp=%d, found=%d)",
		     size, count, cnt);
    }
}

/*
 * Assert that the free list with a specified index has the specified number of
 * blocks in it.
 */
void assert_free_list_size(int index, int size) {
    int cnt = 0;
    sf_block *bp = sf_free_list_heads[index].body.links.next;
    while(bp != &sf_free_list_heads[index]) {
	cnt++;
	bp = bp->body.links.next;
    }
    cr_assert_eq(cnt, size, "Free list %d has wrong number of free blocks (exp=%d, found=%d)",
		 index, size, cnt);
}

Test(sfmm_basecode_suite, malloc_an_int, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	assert_free_block_count(0, 1);
	assert_free_block_count(1952, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(sfmm_basecode_suite, malloc_four_pages, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;

	// We want to allocate up to exactly four pages, so there has to be space
	// for the header and the link pointers.
	void *x = sf_malloc(8092);
	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_basecode_suite, malloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(100281);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(100288, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_basecode_suite, free_no_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	assert_free_block_count(0, 2);
	assert_free_block_count(0, 2);
	assert_free_block_count(224, 1);
	assert_free_block_count(1696, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, free_coalesce, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	assert_free_block_count(0, 2);
	assert_free_block_count(544, 1);
	assert_free_block_count(1376, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(sfmm_basecode_suite, freelist, .timeout = TEST_TIMEOUT) {
        size_t sz_u = 200, sz_v = 300, sz_w = 200, sz_x = 400, sz_y = 200, sz_z = 500;
	void *u = sf_malloc(sz_u);
	/* void *v = */ sf_malloc(sz_v);
	void *w = sf_malloc(sz_w);
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(u);
	sf_free(w);
	sf_free(y);

	assert_free_block_count(0, 4);
	assert_free_block_count(224, 3);
	assert_free_block_count(64, 1);

	// First block in list should be the most recently freed block.
	int i = 4;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 8,
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 8);
}

Test(sfmm_basecode_suite, realloc_larger_block, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int), sz_y = 10, sz_x1 = sizeof(int) * 20;
	void *x = sf_malloc(sz_x);
	/* void *y = */ sf_malloc(sz_y);
	x = sf_realloc(x, sz_x1);

	cr_assert_not_null(x, "x is NULL!");
	sf_block *bp = (sf_block *)((char *)x - 8);
	cr_assert(bp->header & 0x10, "Allocated bit is not set!");
	cr_assert((bp->header & ~0x1f) == 96,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  bp->header & ~0x1f, 96);

	assert_free_block_count(0, 2);
	assert_free_block_count(32, 1);
	assert_free_block_count(1824, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_splinter, .timeout = TEST_TIMEOUT) {
        size_t sz_x = sizeof(int) * 20, sz_y = sizeof(int) * 16;
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");
	cr_assert(x == y, "Payload addresses are different!");

	sf_block *bp = (sf_block *)((char *)y - 8);
	cr_assert(bp->header & 0x10, "Allocated bit is not set!");
	cr_assert((bp->header & ~0x1f) == 96,
		  "Block size (%ld) not what was expected (%ld)!",
		  bp->header & ~0x1f, 96);

	// There should be only one free block.
	assert_free_block_count(0, 1);
	assert_free_block_count(1888, 1);
}

Test(sfmm_basecode_suite, realloc_smaller_block_free_block, .timeout = TEST_TIMEOUT) {
	size_t sz_x = sizeof(double) * 8, sz_y = sizeof(int);
	void *x = sf_malloc(sz_x);
	void *y = sf_realloc(x, sz_y);

	cr_assert_not_null(y, "y is NULL!");

	sf_block *bp = (sf_block *)((char *)y - 8);
	cr_assert(bp->header & 0x10, "Allocated bit is not set!");
	cr_assert((bp->header & ~0x1f) == 32,
		  "Realloc'ed block size (%ld) not what was expected (%ld)!",
		  bp->header & ~0x1f, 32);

	// After realloc'ing x, we can return a block of size ADJUSTED_BLOCK_SIZE(sz_x) - ADJUSTED_BLOCK_SIZE(sz_y)
	// to the freelist.  This block will go into the main freelist and be coalesced.
	assert_free_block_count(0, 1);
	assert_free_block_count(1952, 1);
}

//############################################
//STUDENT UNIT TESTS SHOULD BE WRITTEN BELOW
//DO NOT DELETE OR MANGLE THESE COMMENTS
//############################################

// Unit test for validate_pointer function
Test(sfmm_utils_suite, validate_pointer) {
	// Simulating malloc operations
	// Valid allocated block
	void *ptr1 = sf_malloc(32);
	// Corrupted allocated block
	void *ptr2 = sf_malloc(32);
	sf_block *cbp = (sf_block *)((sf_header *)ptr2 - 1);
	cbp->header = 0x1;
	// Free block
	void *ptr3 = sf_malloc(32);
	sf_free(ptr3);
	// Offset by 1 byte to misalign
    void *unaligned_ptr = (void *)((char *)ptr1 + 1);
	void *out_of_bounds_ptr = (void *)((char *)sf_mem_end() + 32);


    cr_assert_eq(validate_pointer(NULL), -1, "Expected -1 for NULL pointer");                  	// Test NULL pointer
    cr_assert_eq(validate_pointer(unaligned_ptr), -1, "Expected -1 for unaligned pointer");    	// Test unaligned pointer (e.g., not 32-byte aligned)
    cr_assert_eq(validate_pointer((char *)PROLOGUE_POINTER + MEMROWSIZE), -1,					// Test pointer to prologue area
		"Expected -1 for prologue pointer"); 
    cr_assert_eq(validate_pointer((char *)EPILOGUE_POINTER - MEMROWSIZE), -1,					// Test pointer to epilogue area
		"Expected -1 for epilogue pointer");
	cr_assert_eq(validate_pointer(out_of_bounds_ptr), -1,										// Test pointer out of heap bounds (greater than sf_mem_end)
		"Expected -1 for pointer out of heap bounds");
    cr_assert_eq(validate_pointer(ptr3), -1, "Expected -1 for pointer to free block");    		// Test pointer to free block (not allocated)
	cr_assert_eq(validate_pointer(ptr2), -1,													// Test pointer to block with invalid header (corrupted header)
		"Expected -1 for pointer with corrupted header");
    cr_assert_eq(validate_pointer(ptr1), 0, "Expected 0 for valid pointer");					// Test valid block
}