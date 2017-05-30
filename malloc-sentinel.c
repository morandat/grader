// This define is mandatory to enable RTLD_NEXT
#define _GNU_SOURCE
#include "grader.h"

#include <dlfcn.h>
//#include <malloc.h>
#include <stdlib.h>

#define SENTINEL 0xDEADBEEFCAFEBABE
#define DEAD_SENTINEL 0xCAFEDEADBABEBEEF

#ifndef MS_FD
# define MS_FD stderr
#endif

#define member_size(type, member) \
    sizeof(((type *)0)->member)

#define M_PTR(__p) \
    ((struct m_block*) ((char*)__p - member_size(struct m_block, header)))
#define M_SIZE(__size) \
    (__size + member_size(struct m_block, header))

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
        fprintf(MS_FD,
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

struct m_block {
    struct {
        size_t size;
        long long sentinel;
    } header;
    char data[1]; // 1 is to be compliant with c std.
};

void *malloc(size_t size) {
    INITIALIZE(malloc, size);
    if (!malloc_enabled)
        return real_malloc(size);
    struct m_block *result = real_malloc(M_SIZE(size));
    if (result) {
        malloc_counters.allocated ++;
        result->header.sentinel = SENTINEL;
        result->header.size = size;
        return result->data;
    }
    return result;
}

void *realloc(void* ptr, size_t size) {
    INITIALIZE(realloc, ptr, size);
    if (malloc_enabled)
        return real_realloc(ptr, size);
    if (!ptr)
        return malloc(size);
    struct m_block *p = M_PTR(ptr);
    switch (p->header.sentinel) {
        case SENTINEL:
            malloc_counters.reallocated ++;
            return real_realloc(p, M_SIZE(size));
        break;
        default:
            fprintf(MS_FD, "realloc() on %s pointer\n",
                    (p->header.sentinel == DEAD_SENTINEL) ? "already freed" : "unallocated");
    }
    return NULL;
}

// In this version free errors (fault) can't be count
void free(void* ptr) {
    INITIALIZE(free, ptr);
    if (malloc_enabled) {
        if (ptr == NULL) {
            fprintf(MS_FD, "Freeing (null)\n");
            malloc_counters.fault ++;
        } else {
            struct m_block *p = M_PTR(ptr);
            switch (p->header.sentinel) {
                case SENTINEL:
                    p->header.sentinel = DEAD_SENTINEL;
                    p->header.size = 0;
                    malloc_counters.freed ++;
                    real_free(p);
                    break;
                default:
                    fprintf(MS_FD, "%s: %p\n",
                            (p->header.sentinel == DEAD_SENTINEL) ? "Double free detected" : "Freeing invalid pointer",
                            p->data);
                    malloc_counters.fault ++;
            }
        }
    } else
        real_free(ptr);
}

int posix_memalign(void** ptr, size_t align, size_t size) {
    INITIALIZE(posix_memalign, ptr, align, size);
    int result = real_posix_memalign(ptr, align, size);
    if (malloc_enabled && !result) {
        fprintf(MS_FD, "posix_memalign() is not supported currently in "__FILE__"\n");
        exit(1);
    }
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
    if (!ptr)
        return 0;
    struct m_block *p = M_PTR(ptr);
    if (p->header.sentinel == SENTINEL)
        return p->header.size;
    return 0;
}
