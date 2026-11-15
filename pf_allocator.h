/*
    pf_allocator.h: allocator abstractions

USAGE
    Define this in your source:

    #define PF_ALLOCATOR_IMPLEMENTATION
    #define "pf_allocator.h"

    This library depends on `pf.h`, so that file must be placed alongside it.

HOW TO USE
    See usage.c in the root of the repository for a simple example.

SHORT NAMES
    For convenience of use, the library exposes shorter declarations not
    qualified with the "pf_" prefix. If these conflict with the names in your
    code, define this flag before including the header:

    #define PF_ALLOCATOR_NO_SHORT_NAMES
*/

#ifndef PF_ALLOCATOR_H
#define PF_ALLOCATOR_H

#ifndef PF_ALLOCATOR_NO_SHORT_NAMES
#define allocator_alloc_fn pf_allocator_alloc_fn
#define allocator_remap_fn pf_allocator_remap_fn
#define allocator_free_fn pf_allocator_free_fn
#define allocator_free_all_fn pf_allocator_free_all_fn
#define allocator_vtable_t pf_allocator_vtable_t

#define allocator_t pf_allocator_t

#define mem_alloc pf_mem_alloc
#define mem_remap pf_mem_remap
#define mem_free pf_mem_free
#define mem_free_all pf_mem_free_all

#define malloc_allocator pf_malloc_allocator
#define default_allocator pf_default_allocator

#define arena_allocator_t pf_arena_allocator_t
#define arena_allocator_init pf_arena_allocator_init
#define arena_allocator pf_arena_allocator
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pf.h"

/// ----------------
/// Allocator interface
/// ----------------
/// Sort of based on libghostty-vt's allocator interface

// Allocate a block of memory.
// Returns NULL if an error was encountered.
typedef void *(*pf_allocator_alloc_fn)(void *ctx, size_t len);

// Resize a block, possibly moving it.
// Returns NULL if an error was encountered.
typedef void *(*pf_allocator_remap_fn)(void *ctx, void *ptr, size_t old_len,
                                       size_t new_len);

// Free a block of memory.
typedef void (*pf_allocator_free_fn)(void *ctx, void *ptr);

// Free all memory allocated by an allocator.
typedef void (*pf_allocator_free_all_fn)(void *ctx);

typedef struct {
    // Allocates a block. Required.
    pf_allocator_alloc_fn alloc;

    // Resizes a block and may move it. Optional.
    pf_allocator_remap_fn remap;

    // Frees a block. Required.
    pf_allocator_free_fn free;

    // Frees everything allocated by the allocator. Optional.
    pf_allocator_free_all_fn free_all;
} pf_allocator_vtable_t;

// A custom memory allocator.
// It includes a context with whatever state it keeps, and a pointers to the
// functions that implement this allocator.
typedef struct {
    // The state that this allocator holds, may be NULL.
    void *ctx;

    // The functions that implement this allocator. Must NOT be NULL.
    const pf_allocator_vtable_t *vtable;
} pf_allocator_t;

// Allocate a buffer of `len` bytes.
// Uses the provided allocator, or the default (C `malloc`) if NULL is passed.
void *pf_mem_alloc(const pf_allocator_t *allocator, size_t len);

// Resize the provided memory block, possibly moving it.
// Uses the provided allocator if it implements remap, or the default (C
// `realloc`) if NULL is passed. If neither is true, this function does nothing
// and returns NULL.
void *pf_mem_remap(const pf_allocator_t *allocator, void *ptr, size_t old_len,
                   size_t new_len);

// Free memory allocated at `ptr`.
// Uses the provided allocator, or the default (C `free`) if NULL is passed. If
// neither is true, this is a no-op.
// If `ptr` is NULL this is a no-op.
void pf_mem_free(const pf_allocator_t *allocator, void *ptr);

// Free all memory allocated by `allocator`.
// Uses the provided allocator if it implements free_all. If it doesn't or if
// `allocator` is NULL, this is a no-op.
void pf_mem_free_all(const pf_allocator_t *allocator);

/// ----------------
/// Allocators
/// ----------------

// The usual C heap allocation functions (C `malloc`, `realloc`, `free`) wrapped
// in the allocator interface.
pf_allocator_t pf_malloc_allocator(void);

// The default allocator is a variant of the malloc allocator; it uses those
// same functions, but always zeroes out newly allocated memory.
pf_allocator_t pf_default_allocator(void);

/// Arena allocator

// The arena allocator.
// Allocates on a provided backing buffer, deallocates all at once via
// `free_all`. Calling `free` is a no-op.
// The backing buffer must remain alive at least as long as the arena is alive.
typedef struct {
    uint8_t *buf;
    size_t size;
    size_t curr;
    size_t prev;
} pf_arena_allocator_t;

// Initialize the arena allocator `arena` with a backing buffer.
void pf_arena_allocator_init(pf_arena_allocator_t *arena, void *backing_buf,
                             size_t backing_buf_len);

// Convert the given arena allocator to the generic allocator interface.
pf_allocator_t pf_arena_allocator(pf_arena_allocator_t *arena);

#endif  // PF_ALLOCATOR_H

#ifdef PF_ALLOCATOR_IMPLEMENTATION
#ifndef _PF_ALLOCATOR_IMPLEMENTED
#define _PF_ALLOCATOR_IMPLEMENTED

static void *pf_malloc_allocator_alloc(void *ctx, size_t len)
{
    (void) ctx;
    return malloc(len);
}

static void *pf_malloc_allocator_remap(void *ctx, void *ptr, size_t old_len,
                                       size_t new_len)
{
    (void) ctx;
    (void) old_len;
    return realloc(ptr, new_len);
}

static void pf_malloc_allocator_free(void *ctx, void *ptr)
{
    (void) ctx;
    free(ptr);
}

static const pf_allocator_vtable_t pf_malloc_allocator_vtable = {
    .alloc = pf_malloc_allocator_alloc,
    .remap = pf_malloc_allocator_remap,
    .free = pf_malloc_allocator_free,
    .free_all = NULL,
};

pf_allocator_t pf_malloc_allocator(void)
{
    return (pf_allocator_t) {
        .ctx = NULL,
        .vtable = &pf_malloc_allocator_vtable,
    };
}

static void *pf_default_allocator_alloc(void *ctx, size_t len)
{
    (void) ctx;
    void *buf = malloc(len);
    if (!buf)
        return NULL;
    memset(buf, 0, len);
    return buf;
}

static void *pf_default_allocator_remap(void *ctx, void *ptr, size_t old_len,
                                        size_t new_len)
{
    (void) ctx;
    (void) old_len;
    void *buf = realloc(ptr, new_len);
    if (!buf)
        return NULL;
    if (new_len > old_len)
        memset((uint8_t *) buf + old_len, 0, new_len - old_len);
    return buf;
}

static void pf_default_allocator_free(void *ctx, void *ptr)
{
    (void) ctx;
    free(ptr);
}

static const pf_allocator_vtable_t pf_default_allocator_vtable = {
    .alloc = pf_default_allocator_alloc,
    .remap = pf_default_allocator_remap,
    .free = pf_default_allocator_free,
    .free_all = NULL,
};

pf_allocator_t pf_default_allocator(void)
{
    return (pf_allocator_t) {
        .ctx = NULL,
        .vtable = &pf_default_allocator_vtable,
    };
}

void *pf_mem_alloc(const pf_allocator_t *allocator, size_t len)
{
    pf_allocator_t used =
        allocator != NULL ? *allocator : pf_default_allocator();
    return used.vtable->alloc(used.ctx, len);
}

void *pf_mem_remap(const pf_allocator_t *allocator, void *ptr, size_t old_len,
                   size_t new_len)
{
    pf_allocator_t used =
        allocator != NULL ? *allocator : pf_default_allocator();
    pf_allocator_remap_fn remap = used.vtable->remap;
    if (remap != NULL) {
        return remap(used.ctx, ptr, old_len, new_len);
    } else {
        return NULL;
    }
}

void pf_mem_free(const pf_allocator_t *allocator, void *ptr)
{
    if (ptr == NULL)
        return;
    pf_allocator_t used =
        allocator != NULL ? *allocator : pf_default_allocator();
    used.vtable->free(used.ctx, ptr);
}

void pf_mem_free_all(const pf_allocator_t *allocator)
{
    pf_allocator_t used =
        allocator != NULL ? *allocator : pf_default_allocator();
    pf_allocator_free_all_fn free_all = used.vtable->free_all;
    if (free_all != NULL) {
        free_all(used.ctx);
    } else {
        // do nothing
    }
}

void pf_arena_allocator_init(pf_arena_allocator_t *arena, void *backing_buf,
                             size_t backing_buf_len)
{
    *arena = (pf_arena_allocator_t) {
        .buf = backing_buf,
        .size = backing_buf_len,
        .curr = 0,
        .prev = 0,
    };
}

static void *pf_arena_allocator_alloc_align(pf_arena_allocator_t *arena,
                                            size_t size, size_t alignment)
{
    // Align current offset forward to specified alignment
    uintptr_t curr = (uintptr_t) arena->buf + (uintptr_t) arena->curr;
    uintptr_t offset = pf_align_up_u(curr, alignment);
    offset -= (uintptr_t) arena->buf;

    // Check to see if the backing memory has space left
    if (offset + size <= arena->size) {
        void *ptr = &arena->buf[offset];
        arena->prev = offset;
        arena->curr = offset + size;

        // Zero out new memory
        memset(ptr, 0, size);
        return ptr;
    }

    return NULL;
}

static void *pf_arena_allocator_alloc(void *ctx, size_t len)
{
    pf_arena_allocator_t *arena = (pf_arena_allocator_t *) ctx;
    return pf_arena_allocator_alloc_align(arena, len, PF_DEFAULT_ALIGNMENT);
}

// Doesn't do anything, just here for completeness.
static void pf_arena_allocator_free(void *ctx, void *ptr)
{
    (void) ctx;
    (void) ptr;
}

static void *pf_arena_allocator_remap_align(pf_arena_allocator_t *arena,
                                            void *ptr, size_t old_size,
                                            size_t new_size, size_t alignment)
{
    uint8_t *old_memory = (uint8_t *) ptr;

    // If the provided pointer is null or the size is zero, just allocate
    // directly
    if (old_memory == NULL || old_size == 0) {
        return pf_arena_allocator_alloc_align(arena, new_size, alignment);
    }

    // Is the pointer within the arena?
    else if (arena->buf <= old_memory &&
             old_memory < (arena->buf + arena->size)) {
        // Is the previous allocation the one being resized?
        // If so we can just grow or shrink directly
        if (arena->buf + arena->prev == old_memory) {
            if (arena->prev + new_size > arena->size) {
                // Out of memory
                return NULL;
            }
            arena->curr = arena->prev + new_size;
            if (new_size > old_size) {
                // Zero out new memory, again
                memset(&arena->buf[arena->prev + old_size], 0,
                       new_size - old_size);
            }
            return old_memory;
        }
        // Otherwise we must allocate new memory
        else {
            void *new_memory =
                pf_arena_allocator_alloc_align(arena, new_size, alignment);
            if (new_memory == NULL)
                return NULL;
            size_t copy_size = old_size < new_size ? old_size : new_size;
            // Copy old memory to new
            memcpy(new_memory, old_memory, copy_size);
            return new_memory;
        }
    } else {
        // Invalid pointer
        // TODO: figure out a way to return better errors rather than just NULL
        return NULL;
    }
}

static void *pf_arena_allocator_remap(void *ctx, void *ptr, size_t old_len,
                                      size_t new_len)
{
    pf_arena_allocator_t *arena = (pf_arena_allocator_t *) ctx;
    return pf_arena_allocator_remap_align(arena, ptr, old_len, new_len,
                                          PF_DEFAULT_ALIGNMENT);
}

static void pf_arena_allocator_free_all(void *ctx)
{
    pf_arena_allocator_t *arena = (pf_arena_allocator_t *) ctx;
    arena->curr = 0;
    arena->prev = 0;
}

static const pf_allocator_vtable_t pf_arena_allocator_vtable = {
    .alloc = pf_arena_allocator_alloc,
    .remap = pf_arena_allocator_remap,
    .free = pf_arena_allocator_free,
    .free_all = pf_arena_allocator_free_all,
};

pf_allocator_t pf_arena_allocator(pf_arena_allocator_t *arena)
{
    return (pf_allocator_t) {
        .ctx = arena,
        .vtable = &pf_arena_allocator_vtable,
    };
}

#endif  // _PF_ALLOCATOR_IMPLEMENTED
#endif  // PF_ALLOCATOR_IMPLEMENTATION
