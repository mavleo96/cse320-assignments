#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

#include "__grading_helpers.h"

Test(basecode_suite, malloc_an_int_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
	sf_errno = 0;
	size_t sz = sizeof(int);
	int *x = sf_malloc(sz);

	cr_assert_not_null(x, "x is NULL!");

	*x = 4;

	cr_assert(*x == 4, "sf_malloc failed to give proper space for an int!");

	_assert_free_block_count(0, 1);
	_assert_free_block_count(1952, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
	cr_assert(sf_mem_start() + PAGE_SZ == sf_mem_end(), "Allocated more than necessary!");
}

Test(basecode_suite, malloc_four_pages_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
	sf_errno = 0;

	// We want to allocate up to exactly four pages, so there has to be space
	// for the header and the link pointers.
	void *x = sf_malloc(8092);
	cr_assert_not_null(x, "x is NULL!");
	_assert_free_block_count(0, 0);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(basecode_suite, malloc_too_large_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
	sf_errno = 0;
	void *x = sf_malloc(128953);

	cr_assert_null(x, "x is not NULL!");
	_assert_free_block_count(0, 1);
	_assert_free_block_count(128960, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(basecode_suite, free_no_coalesce_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
	sf_errno = 0;
	size_t sz_x = 8, sz_y = 200, sz_z = 1;
	/* void *x = */ sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);

	_assert_free_block_count(0, 2);
	_assert_free_block_count(0, 2);
	_assert_free_block_count(224, 1);
	_assert_free_block_count(1696, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(basecode_suite, free_coalesce_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
	sf_errno = 0;
	size_t sz_w = 8, sz_x = 200, sz_y = 300, sz_z = 4;
	/* void *w = */ sf_malloc(sz_w);
	void *x = sf_malloc(sz_x);
	void *y = sf_malloc(sz_y);
	/* void *z = */ sf_malloc(sz_z);

	sf_free(y);
	sf_free(x);

	_assert_free_block_count(0, 2);
	_assert_free_block_count(544, 1);
	_assert_free_block_count(1376, 1);

	cr_assert(sf_errno == 0, "sf_errno is not zero!");
}

Test(basecode_suite, freelist_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
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

	_assert_free_block_count(0, 4);
	_assert_free_block_count(224, 3);
	_assert_free_block_count(64, 1);

	// First block in list should be the most recently freed block.
	int i = 4;
	sf_block *bp = sf_free_list_heads[i].body.links.next;
	cr_assert_eq(bp, (char *)y - 8,
		     "Wrong first block in free list %d: (found=%p, exp=%p)",
                     i, bp, (char *)y - 8);
}

Test(basecode_suite, realloc_larger_block_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
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

	_assert_free_block_count(0, 2);
	_assert_free_block_count(32, 1);
	_assert_free_block_count(1824, 1);
}

Test(basecode_suite, realloc_smaller_block_splinter_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
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
	_assert_free_block_count(0, 1);
	_assert_free_block_count(1888, 1);
}

Test(basecode_suite, realloc_smaller_block_free_block_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment) {
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
	_assert_free_block_count(0, 1);
	_assert_free_block_count(1952, 1);
}
