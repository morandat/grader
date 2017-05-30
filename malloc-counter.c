// This define is mandatory to enable RTLD_NEXT
#define _GNU_SOURCE
#include "grader.h"

#include <dlfcn.h>
#include <stdlib.h>

static void* (*real_malloc)(size_t size) = 0;
static void* (*real_realloc)(void* ptr, size_t size) = 0;
static int  (*real_posix_memalign)(void **ptr, size_t align, size_t size) = 0;
static void  (*real_free)(void* ptr) = 0;

static struct a_stats malloc_counters;

static int malloc_enabled;
static int initialized;

static void initialize() {
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

void *malloc(size_t size) {
    INITIALIZE(malloc, size);
    void * result = real_malloc(size);
    if (malloc_enabled && result)
        malloc_counters.allocated ++;
    return result;
}

void *realloc(void* ptr, size_t size) {
    INITIALIZE(realloc, ptr, size);
    void * result = real_realloc(ptr, size);
    if (malloc_enabled && result) {
        if (ptr)
            malloc_counters.reallocated ++;
        else
            malloc_counters.allocated ++;
    }
    return result;
}

// In this version free errors (fault) can't be count
void free(void* ptr) {
    INITIALIZE(free, ptr);
    if (malloc_enabled)
        malloc_counters.freed ++;
    real_free(ptr);
}

int posix_memalign(void** ptr, size_t align, size_t size) {
    INITIALIZE(posix_memalign, ptr, align, size);
    int result = real_posix_memalign(ptr, align, size);
    if (malloc_enabled && !result)
        malloc_counters.allocated ++;
    return result;
}

///// Public API

struct a_stats get_a_stats() {
    return malloc_counters;
}

void enable_malloc_protector() {
    malloc_enabled = 1;
}

void disable_malloc_protector() {
    malloc_enabled = 0;
}

void free_all() {
    perror("free_all() is not available in " __FILE__);
}

size_t pointer_info(void *ptr) {
  (void) ptr;
  perror("pointer_info() is not available in " __FILE__);
  return 0;
}
