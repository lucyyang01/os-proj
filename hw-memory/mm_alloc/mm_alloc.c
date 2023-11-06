/*
 * mm_alloc.c
 */

#include "mm_alloc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

static struct memory_block_node* mem_head = NULL;

static struct memory_block_node* mem_tail = NULL;


void* mm_malloc(size_t size) {
  //TODO: Implement malloc

  if (size == 0)
    return NULL;
  //initialize list if it's empty
  if (mem_head == NULL) {
    mem_head = sbrk(sizeof(struct memory_block_node));
    mem_head->prev = NULL;
    mem_head->free = false;
    mem_head->next = NULL;
    mem_head->size = size;
    mem_head->allocated = sbrk(size);
    mem_tail = mem_head;
    return mem_head->allocated;
  }

  //iterate through mem to see if there's a block big enough
  struct memory_block_node* curr = mem_head;
  while(curr != NULL) {
    //if we find a sufficiently large block
    if(curr->size >= size) {
      //if the current block can hold anohter block
      if (curr->size > size + sizeof(struct memory_block_node)) {
        //split the current block
        struct memory_block_node* new_block = sbrk(sizeof(struct memory_block_node));
        new_block->prev = mem_tail;
        new_block->next = NULL;
        new_block->size = curr->size - size - sizeof(struct memory_block_node);
        new_block->free = true;
        new_block->allocated = sbrk(size);
        memset(new_block->allocated, 0, curr->size - size - sizeof(struct memory_block_node));
        return curr->allocated;
      }
      //current block can't accommodate another block
      return curr->allocated;
    }
    curr = curr->next;
  }

  //if we haven't returned by this point we need to allocate a new block
  struct memory_block_node* new_block1 = sbrk(sizeof(struct memory_block_node));
  new_block1->prev = mem_tail;
  mem_tail->next = new_block1;
  new_block1->next = NULL;
  new_block1->size = size;
  new_block1->free = false;
  new_block1->allocated = sbrk(size);
  memset(new_block1->allocated, 0, size);
  new_block1 = mem_tail;
  return new_block1->allocated;
}

void* mm_realloc(void* ptr, size_t size) {
  //TODO: Implement realloc

  return NULL;
}

void mm_free(void* ptr) {
  //TODO: Implement free
}
