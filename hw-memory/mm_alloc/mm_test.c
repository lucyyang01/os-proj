#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

/* Function pointers to hw3 functions */
void* (*mm_malloc)(size_t);
void* (*mm_realloc)(void*, size_t);
void (*mm_free)(void*);

static void* try_dlsym(void* handle, const char* symbol) {
  char* error;
  void* function = dlsym(handle, symbol);
  if ((error = dlerror())) {
    fprintf(stderr, "%s\n", error);
    exit(EXIT_FAILURE);
  }
  return function;
}

static void load_alloc_functions() {
  void* handle = dlopen("hw3lib.so", RTLD_NOW);
  if (!handle) {
    fprintf(stderr, "%s\n", dlerror());
    exit(EXIT_FAILURE);
  }

  mm_malloc = try_dlsym(handle, "mm_malloc");
  mm_realloc = try_dlsym(handle, "mm_realloc");
  mm_free = try_dlsym(handle, "mm_free");
}

int main() {
  load_alloc_functions();

        //   I malloc two arrays of the same size, P and Q.
        // Then, I realloc P to be twice as large. Since P and Q should be adjacent, realloc() should move P to a new position.
        // Finally, I malloc an array S that is the same size as P.
        // S should occupy the memory previously used by P.
        // Then, I try to realloc P to be too large to fit in memory.
        // P should not be reallocated, since there is not enough memory.
        // The existing memory containing P should not be affeted.
        // I free all my memory.

  int* p = mm_malloc(50);
  int* q = mm_malloc(50);
  assert(p != q);

  int* rp = mm_realloc(p, 100);
  assert(p != rp);
  int* save_rp = rp;

  int* s = mm_malloc(50);
  assert(s == p);
  int* largep = mm_realloc(p, 100239842947239847);
  assert(largep == NULL);
  assert(rp == p);
  puts("simple realloc");


  // int* data = mm_malloc(sizeof(int));
  // assert(data != NULL);
  // data[0] = 0x162;
  // mm_free(data);
  // puts("malloc test successful!");

  // puts("malloc 2 big blocks");
  // int* a = mm_malloc(100);
  // assert(a != NULL);
  // int* b = mm_malloc(100);
  // assert(b != NULL);
  // assert(a != b);
  // //int* old_a = a;
  // //printf("%d\n", *old_a);
  
  // //puts("free a");
  // //printf("I REACHED HERE");
  // //printf("I REACHED HERE");
  // mm_free(a);

  // //c, d evaluating to same address
  // puts("malloc 2 small blocks");
  // int* c = mm_malloc(8);
  // printf("%d\n", *c);
  // assert(c != NULL);
  // int* d = mm_malloc(8);
  // assert(d != NULL);
  // assert(c == a);
  // //printf("%d\n", *c);
  // puts("block split successful");
  // I call p = malloc(0).
  //       Then I call free(p).
  //       Then I call free(NULL).
  //       Then I call realloc(NULL, 64).
  //       Then I call realloc(NULL, 0).
  //       Then I call realloc(ptr, 0), where ptr is a valid pointer.
  //       I check that ptr has been freed.


  // int* a = mm_malloc(10);
  // int* b = mm_malloc(10);
  // assert(a != b);

  // mm_free(a);
  // mm_free(b);

  // int* c = mm_malloc(4);
  // assert(a == c);
  // mm_free(c);
  // puts("simple coalesce");
}
