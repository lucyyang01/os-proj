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
  //printf("size: %d\n", size);

  //iterate through mem to see if there's a block big enough
  struct memory_block_node* curr = mem_head;
  while(curr != NULL) {
    // printf("curr size: %d\n", curr->size);
    // printf("curr free: %d\n", curr->free);
    //if we find a sufficiently large block and it's free
    if(curr->size >= size && curr->free == true) {
      //printf("I REACHED HERE");
      //if the current block can hold anohter block
      if (curr->size >= 1 + size + sizeof(struct memory_block_node)) {
        printf("i split");
        //split the current block
        struct memory_block_node* new_block = sbrk(sizeof(struct memory_block_node));
        if (new_block == (void*) -1)
          return NULL;
        //update curr
        __uint32_t old_size = curr->size;
        curr->size = size;
        curr->free = false;
        //populate new block
        struct memory_block_node* old_next = curr->next;
        new_block->prev = curr;
        new_block->next = old_next;
        old_next->prev = new_block;
        curr->next = new_block;
        new_block->size =  old_size - (sizeof(struct memory_block_node) - (new_block->size));
        new_block->free = true;
        new_block->allocated = sbrk(new_block->size);
        if (new_block->allocated == (void*) -1)
          return NULL;
        memset(new_block->allocated, 0, new_block->size);
        return curr->allocated;


        // struct memory_block_node* old_next = curr->next;
        // new_block->prev = curr;
        // new_block->next = old_next;
        // old_next->prev = new_block;
        // curr->next = new_block;
        // new_block->size = size;
        // new_block->free = false;
        // new_block->allocated = sbrk(size);
        // if (new_block->allocated == (void*) -1)
        //   return NULL;
        // memset(new_block->allocated, 0, new_block->size);
        // curr->size -= sizeof(struct memory_block_node) + (new_block->size);
        // return new_block->allocated;
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
  //TODO: Implement realloc

  return NULL;
}

void mm_free(void* ptr) {
  // //TODO: Implement free
  //printf("I REACHED HERE");
  if(ptr == NULL) 
    return;
  //find the block of memory ptr corresponds to 
  //printf("I REACHED HERE");
  struct memory_block_node* curr = mem_head;
  while(curr != NULL) {
    //printf("I REACHED HERE");
    if (curr->allocated == ptr) {
      //free the block
      curr->free = true;
      memset(curr->allocated, 0, curr->size);
      //printf("I REACHED HERE");
      //coalesce one at a time
      //check left block
      if ((curr->prev != NULL) && (curr->prev->free == true)) {
        curr->prev->size += (sizeof(struct memory_block_node) + curr->size);
        curr->prev->next = curr->next;
        if(curr->next) {
          curr->next->prev = curr->prev;
        }
        curr = curr->prev;
      }
      //check right block
      if((curr->next != NULL) && (curr->next->free == true)) {
        //if left block has been coalesced, curr is now the left block
        curr->size += (sizeof(struct memory_block_node) + curr->next->size);
        curr->next = curr->next->next;
        if(curr->next->next) {
          curr->next->next->prev = curr;
        }
      }

      //coalesce multiple contiguous blocks of memory
    }
    curr = curr->next;
  }





  //COALESCE ONE AT A TIME


  // //find the block to free
  // struct memory_block_node* curr = mem_head;
  // while(curr != NULL) {
  //   if (curr->allocated == ptr) {
  //     curr->free = true;
  //     //if next consecutive block free
  //     if (curr->next->free == true) {
  //       struct memory_block_node* old = curr->next;
  //       curr->next = curr->next->next;
  //       old = NULL;
  //     }
  //     //if next and prev are free
  //     // if (curr->next->free == curr->prev->free == true) {
  //     //   curr->prev->next = curr->next;
  //     //   curr->next->prev = curr->prev;
  //     //   curr->next = NULL;
  //     //   curr->prev = NULL;
  //     // }
  //     // //if just next is free
  //     // if (curr->next->free == true) {
  //     //   curr->next = curr->next->next;
  //     //   curr->next->prev = NULL;
  //     //   curr->next->next = NULL;
  //     // } 
  //     // if (curr->prev->free == true) {
  //     //   curr->prev = curr->prev->prev;
  //     //   curr->prev->prev = NULL;
  //     //   curr->prev->next = NULL;
  //     // } 
  //     // break;    
  //     break;
  //   }
  //   curr = curr->next;
  // }

}
