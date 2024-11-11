#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

#include "__grading_helpers.h"

/*
 * Do one malloc and check that the prologue and epilogue are correctly initialized
 */
Test(initialization_suite, initialization_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	size_t sz = 1;
	void *p  = sf_malloc(sz);
	cr_assert(p != NULL, "The pointer should NOT be null after a malloc");
	_assert_heap_is_valid();
}

/*
 * Single malloc tests, up to the size that forces a non-minimum block size.
 */
Test(single_malloc_suite, malloc_1_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{

	size_t sz = 1;
	void *x = sf_malloc(sz);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 32);
	_assert_heap_is_valid();
	_assert_free_block_count(1952, 1);
	_assert_errno_eq(0);
}

Test(single_malloc_suite, malloc_16_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	size_t sz = 16;
	void *x = sf_malloc(sz);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 32);
	_assert_heap_is_valid();
	_assert_free_block_count(1952, 1);
	_assert_errno_eq(0);
}

Test(single_malloc_suite, malloc_32_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	size_t sz = 32;
	void *x = sf_malloc(sz);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 64);
	_assert_heap_is_valid();
	_assert_free_block_count(1920, 1);
	_assert_errno_eq(0);
}

/*
 * Single malloc test, of a size exactly equal to what is left after initialization.
 * Requesting the exact remaining size (leaving space for the header)
 */
Test(single_malloc_suite, exactly_one_page_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	size_t sz = 1976;
	void *x = sf_malloc(sz);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 1984);
	_assert_heap_is_valid();
	_assert_free_block_count(0, 0);
	_assert_errno_eq(0);
}

/*
 * Single malloc test, of a size just larger than what is left after initialization.
 */
Test(single_malloc_suite, more_than_one_page_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	size_t sz = 2008;
	void *x = sf_malloc(sz);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 2016);
	_assert_heap_is_valid();
	_assert_free_block_count(2016, 1);
	_assert_errno_eq(0);
}

/*
 * Single malloc test, of multiple pages.
 */
Test(single_malloc_suite, three_pages_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(6000);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 6016);
	_assert_heap_is_valid();
	_assert_free_block_count(64, 1);
	_assert_errno_eq(0);
}

/*
 * Single malloc test, unsatisfiable.
 * There should be one single large block.
 */
Test(single_malloc_suite, malloc_max_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	size_t sz = 128953;
	void *x = sf_malloc(sz);
	_assert_null_payload_pointer(x);
	_assert_heap_is_valid();
	_assert_free_block_count(128960, 1);
	_assert_errno_eq(ENOMEM);
}

/*
 * Malloc/free with/without coalescing.
 */
Test(malloc_free_suite, no_coalesce_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(200);
	_assert_nonnull_payload_pointer(x);
	void *y = sf_malloc(300);
	_assert_nonnull_payload_pointer(y);
	void *z = sf_malloc(400);
	_assert_nonnull_payload_pointer(z);
	sf_free(y);
	_assert_block_info(x - 8, 1, 224);
	_assert_block_info(y - 8, 0, 320);
	_assert_block_info(z - 8, 1, 416);
	_assert_heap_is_valid();
	_assert_free_block_count(320, 1);
	_assert_free_block_count(1024, 1);
	_assert_errno_eq(0);
}

Test(malloc_free_suite, coalesce_lower_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(200);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 224);
	void *y = sf_malloc(300);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y - 8, 1, 320);
	void *z = sf_malloc(400);
	_assert_nonnull_payload_pointer(z);
	_assert_block_info(z - 8, 1, 416);
	void *w = sf_malloc(500);
	_assert_nonnull_payload_pointer(w);
	_assert_block_info(w - 8, 1, 512);
	sf_free(y);
	sf_free(z);
	_assert_block_info(x - 8, 1, 224);
	_assert_block_info(y - 8, 0, 736);
	_assert_block_info(w - 8, 1, 512);
	_assert_heap_is_valid();
	_assert_free_block_count(736, 1);
	_assert_free_block_count(512, 1);
	_assert_errno_eq(0);
}

Test(malloc_free_suite, coalesce_upper_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(200);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 224);
	void *y = sf_malloc(300);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y - 8, 1, 320);
	void *z = sf_malloc(400);
	_assert_nonnull_payload_pointer(z);
	_assert_block_info(z - 8, 1, 416);
	void *w = sf_malloc(500);
	_assert_nonnull_payload_pointer(w);
	_assert_block_info(w - 8, 1, 512);
	sf_free(z);
	sf_free(y);
	_assert_block_info(x - 8, 1, 224);
	_assert_block_info(y - 8, 0, 736);
	_assert_block_info(w - 8, 1, 512);
	_assert_heap_is_valid();
	_assert_free_block_count(736, 1);
	_assert_free_block_count(512, 1);
	_assert_errno_eq(0);
}

Test(malloc_free_suite, coalesce_both_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(200);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 224);
	void *y = sf_malloc(300);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y - 8, 1, 320);
	void *z = sf_malloc(400);
	_assert_nonnull_payload_pointer(z);
	_assert_block_info(z - 8, 1, 416);
	void *w = sf_malloc(500);
	_assert_nonnull_payload_pointer(w);
	_assert_block_info(w - 8, 1, 512);
	sf_free(x);
	sf_free(z);
	sf_free(y);
	_assert_block_info(x - 8, 0, 960);
	_assert_heap_is_valid();
	_assert_free_block_count(960, 1);
	_assert_free_block_count(512, 1);
	_assert_errno_eq(0);
}

Test(malloc_free_suite, first_block_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(200);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 224);
	void *y = sf_malloc(300);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y - 8, 1, 320);
	sf_free(x);
	_assert_block_info(x - 8, 0, 224);
	_assert_block_info(y - 8, 1, 320);
	_assert_heap_is_valid();
	_assert_free_block_count(224, 1);
	_assert_free_block_count(1440, 1);
	_assert_errno_eq(0);
}

Test(malloc_free_suite, last_block_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(200);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 224);
	void *y = sf_malloc(1752);
	_assert_nonnull_payload_pointer(y);
	_assert_block_info(y - 8, 1, 1760);
	sf_free(y);
	_assert_block_info(x - 8, 1, 224);
	_assert_block_info(y - 8, 0, 1760);
	_assert_free_block_count(1760, 1);
	_assert_heap_is_valid();
	_assert_errno_eq(0);
}

/*
 * Check that malloc leaves no splinter.
 */
Test(single_malloc_suite, splinter_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
	void *x = sf_malloc(1945);
	_assert_nonnull_payload_pointer(x);
	_assert_block_info(x - 8, 1, 1984);
	_assert_heap_is_valid();
	_assert_free_block_count(0, 0);
	_assert_errno_eq(0);
}

/*
 * Determine if the existing heap can satisfy an allocation request
 * of a specified size.  The heap blocks are examined directly;
 * freelists are ignored.
 */
static int can_satisfy_request(size_t size) {
    size_t asize = (size + 39) >> 5 << 5;
    if(asize < 32)
        asize = 32;
    sf_block *bp = (sf_block *)(sf_mem_start() + 32 + slop());
    while(bp->header & ~0x1f) {
	if(!(bp->header & 0x10) && asize <= (bp->header & ~0x1f))
	    return 1;  // Suitable free block found.
	bp = (sf_block *)((char *)bp + (bp->header & ~0x1f));
    }
    return 0;  // No suitable free block.
}

/*
 *  Allocate small blocks until memory exhausted.
 */
Test(stress_suite, malloc_to_exhaustion_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
    size_t size = 90;
    size_t asize = 128;
    int limit = 10000;
    void *x;
    size_t bsize = 0;
    while(limit > 0 && (x = sf_malloc(size)) != NULL) {
	sf_block *bp = x - 8;
	bsize = bp->header & ~0x1f;
	cr_assert(!bsize || bsize - asize < 32,
		  "Block has incorrect size (was: %lu, exp range: [%lu, %lu))",
		  bsize, asize, 160);
	limit--;
    }
    cr_assert_null(x, "Allocation succeeded, but heap should be exhausted.");
    _assert_errno_eq(ENOMEM);
    cr_assert_null(sf_mem_grow(), "Allocation failed, but heap can still grow.");
    cr_assert(!can_satisfy_request(size),
	      "Allocation failed, but there is still a suitable free block.");
}

/*
 *  Test sf_memalign handling invalid arguments:
 *  If align is not a power of two or is less than the minimum block size,
 *  then NULL is returned and sf_errno is set to EINVAL.
 *  If size is 0, then NULL is returned without setting sf_errno.
 */
Test(memalign_suite, test_1a_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
    /* Test size is 0, then NULL is returned without setting sf_errno */
    sf_errno = ENOTTY;  // Set errno to something it will never be set to as a test (in this case "not a typewriter")
    size_t arg_align = 32;
    size_t arg_size = 0;
    void *actual_ptr = sf_memalign(arg_size, arg_align);
    cr_assert_null(actual_ptr, "sf_memalign didn't return NULL when passed a size of 0");
    _assert_errno_eq(ENOTTY);  // Assert that the errno didn't change
}

Test(memalign_suite, test_1b_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
    /* Test align less than the minimum block size */
    size_t arg_align = 1U << 2;  // A power of 2 that is still less than MIN_BLOCK_SIZE
    size_t arg_size = 25;  // Arbitrary
    void *actual_ptr = sf_memalign(arg_size, arg_align);
    cr_assert_null(actual_ptr, "sf_memalign didn't return NULL when passed align that was less than the minimum block size");
    _assert_errno_eq(EINVAL);
}

Test(memalign_suite, test_1c_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
    /* Test align that isn't a power of 2 */
    size_t arg_align = (32 << 1) - 1;  // Greater than 32, but not a power of 2
    size_t arg_size = 65;  // Arbitrary
    void *actual_ptr = sf_memalign(arg_size, arg_align);
    cr_assert_null(actual_ptr, "sf_memalign didn't return NULL when passed align that wasn't a power of 2");
    _assert_errno_eq(EINVAL);
}

/*
Test that memalign returns an aligned address - using minimum block size for alignment
 */
Test(memalign_suite, test_2_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
    size_t arg_align = 32;
    size_t arg_size = 25;  // Arbitrary
    void *x = sf_memalign(arg_size, arg_align);
    _assert_nonnull_payload_pointer(x);
    if (((unsigned long)x & (arg_align-1)) != 0) {
        cr_assert(1 == 0, "sf_memalign didn't return an aligned address!");
    }
}

/*
Test that memalign returns aligned, usable memory
 */
Test(memalign_suite, test_3_alt, .timeout = TEST_TIMEOUT, .init=set_16_byte_alignment)
{
    size_t arg_align = 32<<1; // Use larger than minimum block size for alignment
    size_t arg_size = 129;  // Arbitrary
    void *x = sf_memalign(arg_size, arg_align);
    _assert_nonnull_payload_pointer(x);
    if (((unsigned long)x & (arg_align-1)) != 0) {
        cr_assert(1 == 0, "sf_memalign didn't return an aligned address!");
    }
    _assert_heap_is_valid();
}
