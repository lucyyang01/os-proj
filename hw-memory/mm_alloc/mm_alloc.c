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
    if (mem_head == (void*) -1)
      return NULL;
    mem_head->prev = NULL;
    mem_head->free = false;
    mem_head->next = NULL;
    mem_head->size = size;
    mem_head->allocated = sbrk(size);
    if (mem_head->allocated == (void*) -1)
      return NULL;
    mem_tail = mem_head;
    memset(mem_head->allocated, 0, size);
    return mem_head->allocated;
  }

  if (size >= 4294967295)
    return NULL;
  //printf("size: %d\n", size);

  //iterate through mem to see if there's a block big enough
  struct memory_block_node* curr = mem_head;
  while(curr != NULL) {
    if(curr->size >= size && curr->free == true) {
      //if the current block can hold anohter block
      if (curr->size >= 1 + size + sizeof(struct memory_block_node)) {
        //split the current block
        //update curr
        __uint32_t old_size = curr->size;
        curr->size = size;
        curr->free = false;
        //populate new block
        struct memory_block_node* new_block = curr->allocated + curr->size;
        struct memory_block_node* old_next = curr->next;
        new_block->prev = curr;
        new_block->next = old_next;
        if (old_next) {
          old_next->prev = new_block;
        }
        curr->next = new_block;
        new_block->size =  old_size - sizeof(struct memory_block_node) - size;
        new_block->free = true;
        new_block->allocated = new_block + sizeof(struct memory_block_node);

        memset(new_block->allocated, 0, new_block->size);
        return curr->allocated;
      }
      //current block can't accommodate another block
      return curr->allocated;
    }
    curr = curr->next;
  }

  //if we haven't returned by this point we need to allocate a new block
  struct memory_block_node* new_block1 = sbrk(sizeof(struct memory_block_node));
  if (new_block1 == (void*) -1)
    return NULL;
  new_block1->prev = mem_tail;
  mem_tail->next = new_block1;
  new_block1->next = NULL;
  new_block1->size = size;
  new_block1->free = false;
  new_block1->allocated = sbrk(size);
  if (new_block1->allocated == (void*) -1)
    return NULL;
  memset(new_block1->allocated, 0, size);
  mem_tail = new_block1;
  return new_block1->allocated;
}

void* mm_realloc(void* ptr, size_t size) {
  if(size == 0) {
    mm_free(ptr);
    return NULL;
  }
  if(ptr == NULL) {
    if (size == 0)
      return NULL;
    return mm_malloc(size);
  }  
  struct memory_block_node* curr = mem_head;
  while(curr != NULL) {
    if (curr->allocated == ptr) 
      break;
    curr = curr->next;
    //copy size of old block if size > odl size, otherwise only copy size
  }
  //curr contains the block we wanna realloc
  int* new_ptr = mm_malloc(size);
  if (new_ptr == NULL) {
    return NULL;
  }
  //if we are allocating to a larger size
  if(curr->size < size) 
    memcpy(new_ptr, curr->allocated, curr->size);
  //if we are allocating to a smaller size
  else
    memcpy(new_ptr, curr->allocated, size);
  //memcpy old data to block
  mm_free(curr->allocated);
  return new_ptr;
}

void mm_free(void* ptr) {
  // //TODO: Implement free
  if(ptr == NULL) 
    return;
  //find the block of memory ptr corresponds to 
  struct memory_block_node* curr = mem_head;
  while(curr != NULL) {
    if (curr->allocated == ptr) {
      //free the block
      curr->free = true;
      memset(curr->allocated, 0, curr->size);

      if ((curr->prev != NULL) && (curr->prev->free == true)) {
        curr->prev->size += (sizeof(struct memory_block_node) + curr->size);
        curr->prev->next = curr->next; //curr->next may not exist if this is the last node
        if (curr->next) {
          curr->next->prev = curr->prev;
        }
        curr = curr->prev;
        memset(curr->allocated, 0, curr->size);
      }
      //check right block
      if((curr->next != NULL) && (curr->next->free == true)) {
        curr->size += (sizeof(struct memory_block_node) + curr->next->size);
        if(curr->next->next) {
          curr->next->next->prev = curr;
        }
        curr->next = curr->next->next;
        memset(curr->allocated, 0, curr->size);
      }
    }
    curr = curr->next;
  }
}
