/*
 * Copyright 2021 Stanislav Paskalev <spaskalev@protonmail.com>
 *
 * A binary buddy memory allocator
 *
 * To include and use it in your project do the following
 * 1. Add buddy_alloc.h (this file) to your include directory
 * 2. Include the header in places where you need to use the allocator
 * 3. In one of your source files #define BUDDY_ALLOC_IMPLEMENTATION
 *    and then import the header. This will insert the implementation.
 *
 * Latest version is available at https://github.com/spaskalev/buddy_alloc
 */

#ifndef BUDDY_ALLOC_H
#define BUDDY_ALLOC_H

// #include "../_def.h"
#include <mimalloc.h>
// #include <stdalign.h>
// #include <stdbool.h>
// #include <stddef.h>
// #include <stdint.h>
// #include <string.h>

struct buddy;

// /* Returns the size of a buddy required to manage a block of the specified size */
// size_t buddy_sizeof(size_t memory_size);

// /*
//  * Returns the size of a buddy required to manage a block of the specified size
//  * using a non-default alignment.
//  */
// size_t buddy_sizeof_alignment(size_t memory_size, size_t alignment);

// /* Initializes a binary buddy memory allocator at the specified location */
// struct buddy* buddy_init(unsigned char* at, unsigned char* main, size_t memory_size);

// /* Initializes a binary buddy memory allocator at the specified location using a non-default alignment */
// struct buddy* buddy_init_alignment(unsigned char* at, unsigned char* main, size_t memory_size, size_t alignment);

/*
 * Initializes a binary buddy memory allocator embedded in the specified arena.
 * The arena's capacity is reduced to account for the allocator metadata.
 */
struct buddy* buddy_embed(unsigned char* main, size_t memory_size);
/*
 * Allocation functions
 */
/* Use the specified buddy to allocate memory. See malloc. */
void* buddy_malloc(struct buddy* buddy, size_t requested_size);

/* Use the specified buddy to free memory. See free. */
void buddy_free(struct buddy* buddy, void* ptr);

#endif