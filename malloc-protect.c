// This define is mandatory to enable RTLD_NEXT
#define _GNU_SOURCE
#include "grader.h"

#include <dlfcn.h>
//#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

static void* (*real_malloc)(size_t size) = 0;
static void* (*real_realloc)(void* ptr, size_t size) = 0;
static int  (*real_posix_memalign)(void **ptr, size_t align, size_t size) = 0;
static void  (*real_free)(void* ptr) = 0;

static int malloc_enabled;
static int initialized;

struct a_list {
  void *ptr; // Allocated block
  size_t size; // Allocated size
  struct a_list *next; // Next in allocated chain
};

struct a_block {
  struct a_list *used_list, *free_list;
  struct a_stats stats;
} malloc_superblock;

static long BLOCK_SIZE; // it's not a constant but should be used like if it was one

static void initialize () {
  BLOCK_SIZE = sysconf(_SC_PAGESIZE) / sizeof(struct a_list) - 1;
}

static void load_func(void **fun_hdl, const char *name) {
    void *ptr = *fun_hdl = dlsym(RTLD_NEXT, name);
    if (!ptr) {
        fprintf(stderr,
                "Can not grab symbol %s: %s\n", name, dlerror());
        exit(1);
    } else if (!initialized) {
        initialized = 1;
        initialize();
    }
}

#define INITIALIZE(name, ...)               \
    do {                                    \
        if (!real_##name)                   \
            load_func((void**)&real_##name, #name); \
    } while(0)
 
static struct a_list *allocate_region(struct a_block *b) {
  struct a_list *new_block = real_malloc(sizeof(new_block) * BLOCK_SIZE);

  int i = BLOCK_SIZE - 1;
  while (i --)
    new_block[i].next = new_block + i + 1;

  b->free_list = new_block;
  return new_block;
}

static struct a_list* assign(struct a_list *info, size_t size, void *ptr) {
  info->size = size;
  info->ptr = ptr;
  return info;
}

static void register_ptr(struct a_block *b, size_t size, void *ptr) {
  struct a_list *n = b->free_list;
  if (n == NULL && !(n = allocate_region(b))) {
      perror("Can't allocate new block");
      exit(EXIT_FAILURE);
  }
  b->free_list = n->next; // update free_list
  n->next = b->used_list; // update used_list
  b->used_list = assign(n, size, ptr); // update cell
  b->stats.allocated ++; // update stat
}

struct a_list* locate_ptr(struct a_block *b, void *ptr) {
  struct a_list* node = b->used_list;

  while(node && node->ptr != ptr)
    node = node->next;

  return node;
}

static struct a_list* unregister_ptr(struct a_block *b, void *ptr) {
  struct a_list *node = b->used_list, *prev = NULL;

  // Locate node (and previous node)
  while(node && node->ptr != ptr)
    node = (prev = node)->next;

  if (node) {
    // Remove a found entry
    if (prev)
        prev->next = node->next;
    else // Patch the used list
        b->used_list = node->next;
    // Update the free list
    node->next = b->free_list;
    b->free_list = node;
    b->stats.freed ++;
  } else
    b->stats.fault ++;
  return node;
}

void * malloc(size_t size) {
  INITIALIZE(malloc, size);
  void * result = real_malloc(size);
  if (malloc_enabled && result)
    register_ptr(&malloc_superblock, size, result);
  return result;
}

int posix_memalign(void** ptr, size_t align, size_t size) {
  INITIALIZE(posix_memalign, ptr, align, size);
  int result = real_posix_memalign(ptr, align, size);
  if (malloc_enabled && !result)
    register_ptr(&malloc_superblock, size, *ptr);
  return result;
}

void free(void* ptr) {
    INITIALIZE(free, ptr);
    if (malloc_enabled && !unregister_ptr(&malloc_superblock, ptr))
      fprintf(stderr, "Can not free %p: pointer is not allocated\n", ptr);
    else
      real_free(ptr);
}

void *realloc(void* ptr, size_t size) {
    INITIALIZE(realloc, ptr, size);
    void * result ;
    if (!malloc_enabled)
      return real_realloc(ptr, size);
    if (!ptr) { // this branch can probably be factorized by a call to malloc, but I'm not sure
      result = real_malloc(size);
      if (malloc_enabled && result)
        register_ptr(&malloc_superblock, size, result);
      return result;
    }
    struct a_list *a_node = locate_ptr(&malloc_superblock, ptr);
    if (a_node) {
      void * result = real_realloc(ptr, size);
      if (result) {
        result = assign(a_node, size, result);
        malloc_superblock.stats.reallocated ++;
      }
    } else {
      fprintf(stderr, "Can not realloc %p: pointer is not allocated\n", ptr);
      result = NULL;
    }
    return result;
}

///// Public API

void free_all() {
    while(malloc_superblock.used_list) {
        real_free(malloc_superblock.used_list->ptr);
        unregister_ptr(&malloc_superblock, malloc_superblock.used_list->ptr);
    }
}

struct a_stats get_a_stats() {
  return malloc_superblock.stats;
}

void enable_malloc_protector() {
    malloc_enabled = 1;
}

void disable_malloc_protector() {
    malloc_enabled = 0;
}

size_t pointer_info(void *ptr) {
    struct a_list *node = locate_ptr(&malloc_superblock, ptr);
    if (node)
        return node->size;
    return 0;
}
