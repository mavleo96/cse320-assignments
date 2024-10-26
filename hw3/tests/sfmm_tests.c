#include <criterion/criterion.h>
#include <errno.h>
#include <signal.h>
#include "debug.h"
#include "sfmm.h"
#include "sfmm_utils.h"
#include "math.h"
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
	cr_assert_eq(cnt, count, "Wrong number of free blocks of size %zu (exp=%d, found=%d)",
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
		  "Realloc'ed block size (%zu) not what was expected (%d)!",
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
		  "Block size (%zu) not what was expected (%d)!",
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
		  "Realloc'ed block size (%zu) not what was expected (%d)!",
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

/*----------------------------------------------*/
/*			  sfmm_utils test suite				*/
/*----------------------------------------------*/

Test(sfmm_utils_suite, validate_pointer, .timeout = TEST_TIMEOUT) {
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

Test(sfmm_utils_suite, expand_heap, .timeout = TEST_TIMEOUT) {
	// Initializing the heap
	sf_malloc(0);
	sf_block *nfbp = expand_heap();
	cr_assert_eq(BLOCKSIZE(nfbp), PAGE_SZ, "Expected free block of size %zu; got %zu",			// Test if returned free block is of correc size
		PAGE_SZ, BLOCKSIZE(nfbp));
	cr_assert_eq(EPILOGUE_POINTER->header, 0x10, "Epilogue is not updated correctly!");			// Test if epilogue is set correctly
}

Test(sfmm_utils_suite, expand_heap_error, .timeout = TEST_TIMEOUT) {
	// Initializing the heap
	sf_malloc(0);
	sf_block *bp;
	// Expanding the heap until memory exhausts
	do {
		bp = expand_heap();
	} while (bp != NULL);
	cr_assert_eq(sf_errno, ENOMEM, "sf_errno not set to ENOMEM!");								// Test if sf_errno is set correctly
}

Test(sfmm_utils_suite, update_block_header, .timeout = TEST_TIMEOUT) {
    // Initialize test memory block
	void *ptr1 = sf_malloc(50); // 64 bytes
	void *ptr2 = sf_malloc(20); // 32 bytes
    sf_block *bp = (sf_block *)((sf_header *)ptr1 - 1);
    sf_block *nbp = (sf_block *)((sf_header *)ptr2 - 1);
	sf_footer *fp = FOOTER_POINTER(bp);
	sf_footer *nfp = FOOTER_POINTER(nbp);

	// Free test block
    update_block_header(nbp, nbp->header & ~ 0b10000);
	sf_header new_header = bp->header & ~ 0b10000;
    update_block_header(bp, new_header);

	cr_assert_eq(bp->header, new_header, "Footer not updated correctly!");            			// Test if header of test block is updated to match the new header
	cr_assert_eq(*fp, new_header, "Footer not updated correctly!");				            	// Test if footer of test block is updated to match the new header
    cr_assert_eq(PREV_ALLOCATED_BIT(nbp), 0, 													// Test if prev_allocated_bit is set correctly in next block
		"Next block's prev_allocated bit was not set correctly!");
    cr_assert_eq(PREV_ALLOCATED_BIT((sf_block *)nfp), 0,										// Test if prev_allocated_bit is set correctly in next block footer
		"Next block's prev_allocated bit was not set in footer correctly!");
}

/*----------------------------------------------*/
/*		      sf_memalign test suite			*/
/*----------------------------------------------*/


Test(sfmm_memalign_suite, memalign_invalid_alignment, .timeout = TEST_TIMEOUT) {
	// Invalid alignment, not a power of 2 and < 32
    sf_errno = 0;
    void *result = sf_memalign(100, 20);
    cr_assert_null(result, "Expected NULL for invalid alignment < 32 or not a power of 2");		// Test if invalid arguments are handled correctly
    cr_assert_eq(sf_errno, EINVAL, "Expected sf_errno to be set to EINVAL");					// Test if sf_errno is set correctly for invalid arguments

	// Invalid alignment, not a power of 2
    sf_errno = 0;
    result = sf_memalign(100, 18);
    cr_assert_null(result, "Expected NULL for invalid alignment that is not a power of 2");		// Test if invalid arguments are handled correctly
    cr_assert_eq(sf_errno, EINVAL, "Expected sf_errno to be set to EINVAL");					// Test if sf_errno is set correctly for invalid arguments
}

Test(sfmm_memalign_suite, memalign_small_allocation_valid_alignment, .timeout = TEST_TIMEOUT) {
	// Request memory with valid alignment
    sf_errno = 0;
    size_t align = 64;
    size_t size = 50;
    void *result = sf_memalign(size, align);

    cr_assert_not_null(result, "Expected non-NULL result for valid alignment and allocation");			// Test if sf_memalign return non-NULL pointer 
    cr_assert((long int)result % align == 0, "Expected pointer to be aligned to %zu bytes", align);		// Test if returned pointer is aligned
    cr_assert_eq(sf_errno, 0, "Expected sf_errno to be 0 for successful allocation");					// Test if sf_errno is 0
}

Test(sfmm_memalign_suite, memalign_large_allocation_valid_alignment, .timeout = TEST_TIMEOUT) {
	// Request memory with valid alignment
    sf_errno = 0;  // Reset errno
    size_t align = 128;
    size_t size = 4000;
    void *result = sf_memalign(size, align);

    cr_assert_not_null(result, "Expected non-NULL result for large allocation with valid alignment");	// Test if sf_memalign return non-NULL pointer 
    cr_assert((long int)result % align == 0, "Expected pointer to be aligned to %zu bytes", align);		// Test if returned pointer is aligned
    cr_assert_eq(sf_errno, 0, "Expected sf_errno to be 0 for successful allocation");					// Test if sf_errno is 0

    // Verify that the block size is greater than or equal to the requested size
    sf_block *bp = (sf_block *)((sf_header *)result - 1);
    cr_assert_geq(BLOCKSIZE(bp), size, "Expected allocated block size to be at least %zu bytes", size);	// Test if returned block is of sufficient size
}

Test(sfmm_memalign_suite, memalign_small_allocation_large_alignment, .timeout = TEST_TIMEOUT) {
	// Request memory with valid alignment
    sf_errno = 0;
    size_t align = 4096;
    size_t size = 50;
    void *result = sf_memalign(size, align);

    cr_assert_not_null(result, "Expected non-NULL result for valid alignment and allocation");			// Test if sf_memalign return non-NULL pointer 
    cr_assert((long int)result % align == 0, "Expected pointer to be aligned to %zu bytes", align);		// Test if returned pointer is aligned
    cr_assert_eq(sf_errno, 0, "Expected sf_errno to be 0 for successful allocation");					// Test if sf_errno is 0
}

Test(sfmm_memalign_suite, memalign_zero, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_memalign(0, 128);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(1984, 1);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_memalign_suite, memalign_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_memalign(100281, 128);

	cr_assert_null(x, "x is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(100288, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}

Test(sfmm_memalign_suite, memalign_align_32, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_memalign(100, 32);

	cr_assert_not_null(x, "x is NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(1856, 1);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}


/*----------------------------------------------*/
/*		      sf_realloc test suite				*/
/*----------------------------------------------*/

Test(sfmm_realloc_suite, realloc_content_intact, .timeout = TEST_TIMEOUT) {
    // Allocate initial memory block and fill with data
    size_t original_size = 64;
    char *original_data = sf_malloc(original_size);
    cr_assert_not_null(original_data, "sf_malloc failed for original allocation!");						// Test if sf_malloc return non-NULL pointer 

    // Fill the block with known pattern
    for (size_t i = 0; i < original_size; i++) {
        original_data[i] = (char)(i % 256);
    }

    // Resize the block with sf_realloc to a larger size
    size_t new_size = 128;
    char *resized_data = sf_realloc(original_data, new_size);
    cr_assert_not_null(resized_data, "sf_realloc failed for resizing to larger size!");					// Test if sf_realloc return non-NULL pointer 

    // Check if the content in the resized block is intact
    for (size_t i = 0; i < original_size; i++) {
        cr_assert_eq(resized_data[i], (char)(i % 256), 													// Test if content is correctly copied 
            "Content was not intact at byte %zu after sf_realloc!", i);
    }

    // Resize the block with sf_realloc to a smaller size
    size_t smaller_size = 32;
    char *shrunk_data = sf_realloc(resized_data, smaller_size);
    cr_assert_not_null(shrunk_data, "sf_realloc failed for resizing to smaller size!");					// Test if sf_realloc return non-NULL pointer 

    // Check if the content in the resized block is still intact
    for (size_t i = 0; i < smaller_size; i++) {
        cr_assert_eq(shrunk_data[i], (char)(i % 256),													// Test if content is correctly copied 
            "Content was not intact at byte %zu after shrinking with sf_realloc!", i);
    }
}

Test(sfmm_realloc_suite, realloc_zero, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(1000);
	void *y = sf_realloc(x, 0);

	cr_assert_null(y, "y is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(1984, 1);
	cr_assert(sf_errno == 0, "sf_errno is not 0!");
}

Test(sfmm_realloc_suite, realloc_too_large, .timeout = TEST_TIMEOUT) {
	sf_errno = 0;
	void *x = sf_malloc(1000);
	void *y = sf_realloc(x, 100281);

	cr_assert_null(y, "y is not NULL!");
	assert_free_block_count(0, 1);
	assert_free_block_count(99264, 1);
	cr_assert(sf_errno == ENOMEM, "sf_errno is not ENOMEM!");
}


/*----------------------------------------------*/
/*		        stress test suite				*/
/*----------------------------------------------*/

#define NUM_ITERATIONS 300
#define MAX_SIZE 1000

// Helper function to run the stress test operations
void stress_test_malloc_realloc_free_memalign() {
    void *pointers[NUM_ITERATIONS];            // Array to store pointers for validation
    size_t sizes[NUM_ITERATIONS];              // Store sizes for each allocation
    size_t alignments[NUM_ITERATIONS];         // Store alignments used in memalign
    
    // Initialize arrays
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        pointers[i] = NULL;
        sizes[i] = 0;
        alignments[i] = 0;
    }

    // Random seed for consistent test runs
    srand(0);

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        int op = rand() % 4;  // Choose a random operation: 0=malloc, 1=realloc, 2=free, 3=memalign
        switch (op) {
            case 0:  // sf_malloc
                sizes[i] = rand() % MAX_SIZE + 1;
                pointers[i] = sf_malloc(sizes[i]);
                cr_assert_not_null(pointers[i], "sf_malloc returned NULL at iteration %d", i);
                break;

            case 1:  // sf_realloc
                if (pointers[i]) {
                    sizes[i] = rand() % MAX_SIZE + 1;
                    pointers[i] = sf_realloc(pointers[i], sizes[i]);
                    cr_assert_not_null(pointers[i], "sf_realloc returned NULL at iteration %d", i);
                }
                break;

            case 2:  // sf_free
                if (pointers[i]) {
                    sf_free(pointers[i]);
                    pointers[i] = NULL;  // Mark as freed
                }
                break;

            case 3:  // sf_memalign
                sizes[i] = rand() % MAX_SIZE + 1;
                alignments[i] = (size_t)pow(2, 5 + rand() % 5);
                pointers[i] = sf_memalign(sizes[i], alignments[i]);
                cr_assert_not_null(pointers[i], "sf_memalign returned NULL at iteration %d", i);
                cr_assert((uintptr_t)pointers[i] % alignments[i] == 0, "Pointer not aligned to %zu at iteration %d", alignments[i], i);
                break;
        }
    }
}

Test(sfmm_stress_suite, stress_test_malloc_realloc_free_memalign, .timeout = TEST_TIMEOUT) {
    stress_test_malloc_realloc_free_memalign();
}

Test(sfmm_stress_suite, stress_test_heap_correctness, .timeout = TEST_TIMEOUT) {
    stress_test_malloc_realloc_free_memalign();
	sf_block *bp = PROLOGUE_POINTER;
	int allocated = 0;
	int block_count = 1;
	int early_exit_flag = 0;

	do {
		cr_assert_not_null(bp, "Block pointer is NULL at %d position!", block_count);				// Test if links are not NULL		
		cr_assert(PREV_ALLOCATED_BIT(bp) == allocated,												// Test if prev_allocated_bit is correct
			"Block has prev_allocated_bit corrupted at %d position!", block_count);
		allocated = ALLOCATED_BIT(bp);
		if (!allocated) {
			cr_assert_eq(*FOOTER_POINTER(bp), bp->header, 											// Test if footer matches header if block is free
				"Block has prev_allocated_bit corrupted at %d position!");
			cr_assert_not_null(bp->body.links.next, "Block at %d position has NULL next link!");	// Test if links are not NULL
			cr_assert_not_null(bp->body.links.prev, "Block at %d position has NULL prev link!");	// Test if links are not NULL
		}
		bp = NEXT_BLOCK_POINTER(bp);
		block_count++;
		if (bp > (sf_block *)sf_mem_end()) {
			early_exit_flag = 1;
			break;
		}
	} while ((bp != EPILOGUE_POINTER) && (block_count < NUM_ITERATIONS));
	cr_assert(early_exit_flag == 0, "Accesing a block out of sf_mem_end");
}

Test(sfmm_stress_suite, stress_test_coalesce_correctness) {
    stress_test_malloc_realloc_free_memalign();
	sf_block *bp = PROLOGUE_POINTER;
	int block_count = 1;
	int early_exit_flag = 0;

	do {
		cr_assert_not_null(bp, "Block pointer is NULL at %d position!", block_count);
		if ((bp == PROLOGUE_POINTER) || (bp == EPILOGUE_POINTER)) {
			block_count++;
			continue;
		}
		if (!ALLOCATED_BIT(bp)) {
			cr_assert_eq(PREV_ALLOCATED_BIT(bp), 1,
				"Found uncoalesced free block at position %d & %d!", block_count - 1, block_count);
			cr_assert_eq(ALLOCATED_BIT(NEXT_BLOCK_POINTER(bp)), 1,
				"Found uncoalesced free block at position %d & %d!", block_count, block_count + 1);
		}
		bp = NEXT_BLOCK_POINTER(bp);
		block_count++;
		if (bp > (sf_block *)sf_mem_end()) {
			early_exit_flag = 1;
			break;
		}
	} while ((bp != EPILOGUE_POINTER) && (block_count < NUM_ITERATIONS));
	cr_assert(early_exit_flag == 0, "Accesing a block out of sf_mem_end");
}
