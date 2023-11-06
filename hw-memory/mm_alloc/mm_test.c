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

  // int* data = mm_malloc(sizeof(int));
  // assert(data != NULL);
  // data[0] = 0x162;
  // mm_free(data);
  // puts("malloc test successful!");

  puts("malloc 2 big blocks");
  int* a = mm_malloc(60);
  assert(a != NULL);
  int* b = mm_malloc(60);
  assert(b != NULL);
  assert(a != b);
  //int* old_a = a;
  //printf("%d\n", *old_a);
  
  //puts("free a");
  //printf("I REACHED HERE");
  //printf("I REACHED HERE");
  mm_free(a);

  //c, d evaluating to same address
  puts("malloc 2 small blocks");
  int* c = mm_malloc(8);
  printf("%d\n", *c);
  assert(c != NULL);
  int* d = mm_malloc(8);
  assert(d != NULL);
  assert(c == a);
  //printf("%d\n", *c);
  puts("block split successful");
}
