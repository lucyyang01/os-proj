/*
 * mm_alloc.h
 *
 * Exports a clone of the interface documented in "man 3 malloc".
 */

#pragma once

#ifndef _malloc_H_
#define _malloc_H_

#include <stdlib.h>
#include <stdbool.h>

/* struct to store blocks of memory */
struct memory_block_node {
    struct memory_block_node* prev; /* prev, next are pointers to adjacent blocks of memory*/
    struct memory_block_node* next;
    bool free; /* if this block of memory is free*/
    __uint32_t size; /* size of allocated memory block */
    __uint8_t allocated; 
};

void* mm_malloc(size_t size);
void* mm_realloc(void* ptr, size_t size);
void mm_free(void* ptr);

#endif
