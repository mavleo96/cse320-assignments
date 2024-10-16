#ifndef SFMM_UTILS_H
#define SFMM_UTILS_H

#include <stddef.h>

int global_;
#define ALIGNMENT 32
#define HEADER_SIZE 8

int get_free_list_index_for_size(size_t size);
int min_blocksize(size_t size);
size_t get_blocksize_from_header(sf_header header);
sf_block *find_in_free_list_i(int index, size_t size);

#endif
