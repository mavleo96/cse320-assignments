#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#define TEST_TIMEOUT 15

#include "__grading_helpers.h"

/*
 * Check LIFO discipline on free list
 */
Test(lifo_suite, malloc_free_lifo, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    size_t asz = 224;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    void * u = sf_malloc(sz);
    _assert_nonnull_payload_pointer(u);
    _assert_block_info(u - 8, 1, asz);
    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, asz);
    void * v = sf_malloc(sz);
    _assert_nonnull_payload_pointer(v);
    _assert_block_info(v - 8, 1, asz);
    void * z = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z);
    _assert_block_info(z - 8, 1, asz);
    void * w = sf_malloc(sz);
    _assert_nonnull_payload_pointer(w);
    _assert_block_info(w - 8, 1, asz);
    sf_free(x);
    sf_free(y);
    sf_free(z);
    void * z1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(z1);
    _assert_block_info(z1 - 8, 1, asz);
    void * y1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y1);
    _assert_block_info(y1 - 8, 1, asz);
    void * x1 = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x1);
    _assert_block_info(x1 - 8, 1, asz);
    cr_assert(x == x1 && y == y1 && z == z1,
      "malloc/free does not follow LIFO discipline");
    _assert_free_block_count(640, 1);
    _assert_errno_eq(0);
}

/*
 * Realloc tests.
 */
Test(realloc_suite, realloc_larger, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    size_t asz = 224;
    size_t nsz = 1024;
    size_t ansz = 1056;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, ansz);
    _assert_free_block_count(asz, 1);
    _assert_free_block_count(704, 1);
    _assert_errno_eq(0);
}

Test(realloc_suite, realloc_smaller, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t asz = 1056;
    size_t nsz = 200;
    size_t ansz = 224;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, ansz);
    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");
    _assert_free_block_count(1760, 1);
    _assert_errno_eq(0);
}

Test(realloc_suite, realloc_same, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t asz = 1056;
    size_t nsz = 1024;
    size_t ansz = 1056;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, ansz);
    cr_assert_eq(x, y, "realloc to same size did not return same payload pointer");
    _assert_free_block_count(928, 1);
    _assert_errno_eq(0);
}

Test(realloc_suite, realloc_splinter, .timeout=TEST_TIMEOUT)
{
    size_t sz = 1024;
    size_t asz = 1056;
    size_t nsz = 1020;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, asz);
    cr_assert_eq(x, y, "realloc to smaller size did not return same payload pointer");
    _assert_free_block_count(928, 1);
    _assert_errno_eq(0);
}

Test(realloc_suite, realloc_size_0, .timeout=TEST_TIMEOUT)
{
    size_t sz = 512;
    size_t asz = 544;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    void * y = sf_malloc(sz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, asz);
    void * z = sf_realloc(x, 0);
    _assert_null_payload_pointer(z);
    _assert_block_info(x - 8, 0, asz);
    // after realloc x to (2) z, x is now a free block
    _assert_free_block_count(asz, 1);
    _assert_free_block_count(896, 1);
    _assert_errno_eq(0);
}

Test(realloc_suite, realloc_preserves_payload, .timeout=TEST_TIMEOUT)
{
    size_t sz = 200;
    size_t asz = 224;
    size_t nsz = 1024;
    size_t ansz = 1056;
    void * x = sf_malloc(sz);
    _assert_nonnull_payload_pointer(x);
    _assert_block_info(x - 8, 1, asz);
    // Set payload
    for(int i = 0; i < sz; i++)
	((struct sf_block *)x)->body.payload[i] = (char) i;
    void * y = sf_realloc(x, nsz);
    _assert_nonnull_payload_pointer(y);
    _assert_block_info(y - 8, 1, ansz);
    // Check payload
    for(int i = 0; i < sz; i++) {
	if((((struct sf_block *)y)->body.payload)[i] != (char) i)
	    cr_assert_fail("Payload of realloc'ed block does not match at byte %d\n", i);
    }
    _assert_free_block_count(asz, 1);
    _assert_free_block_count(704, 1);
    _assert_errno_eq(0);
}


/*
 * Illegal pointer tests.
 */
Test(illegal_pointer_suite, free_null, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    (void) sf_malloc(sz);
    sf_free(NULL);
    cr_assert_fail("SIGABRT should have been received");
}

//This test tests: Freeing a memory that was free-ed already
Test(illegal_pointer_suite, free_unallocated, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void *x = sf_malloc(sz);
    sf_free(x);
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(illegal_pointer_suite, free_block_too_small, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * x = sf_malloc(sz);
    ((struct sf_block *)(x - 8))->header = 0x0UL;
    sf_free(x);
    cr_assert_fail("SIGABRT should have been received");
}

Test(illegal_pointer_suite, free_prev_alloc, .signal = SIGABRT, .timeout = TEST_TIMEOUT)
{
    size_t sz = 1;
    void * w = sf_malloc(sz);
    void * x = sf_malloc(sz);
    ((struct sf_block *)(x - 8))->header &= ~0x8;
    sf_free(x);
    sf_free(w);
    cr_assert_fail("SIGABRT should have been received");
}

// random block assigments. Tried to give equal opportunity for each possible order to appear.
// But if the heap gets populated too quickly, try to make some space by realloc(half) existing
// allocated blocks.
Test(stress_suite, stress_test, .timeout = TEST_TIMEOUT)
{
    errno = 0;

    int order_range = 13;
    int nullcount = 0;

    void * tracked[100];

    for (int i = 0; i < 100; i++)
    {
        int order = (rand() % order_range);
        size_t extra = (rand() % (1 << order));
        size_t req_sz = (1 << order) + extra;

        tracked[i] = sf_malloc(req_sz);
        // if there is no free to malloc
        if (tracked[i] == NULL)
        {
            order--;
            while (order >= 0)
            {
                req_sz = (1 << order) + (extra % (1 << order));
                tracked[i] = sf_malloc(req_sz);
                if (tracked[i] != NULL)
                {
                    break;
                }
                else
                {
                    order--;
                }
            }
        }

        // tracked[i] can still be NULL
        if (tracked[i] == NULL)
        {
            nullcount++;
            // It seems like there is not enough space in the heap.
            // Try to halve the size of each existing allocated block in the heap,
            // so that next mallocs possibly get free blocks.
            for (int j = 0; j < i; j++)
            {
                if (tracked[j] == NULL)
                {
                    continue;
                }
                sf_block * bp = tracked[j] - 8;
                req_sz = (bp->header & ~0x1f) >> 1;
                tracked[j] = sf_realloc(tracked[j], req_sz);
            }
        }
        errno = 0;
    }

    for (int i = 0; i < 100; i++)
    {
        if (tracked[i] != NULL)
        {
            sf_free(tracked[i]);
        }
    }

    _assert_heap_is_valid();
}
