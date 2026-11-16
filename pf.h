/*
    pf.h: essentials for C

USAGE
    Define this in your source:

    #define PF_IMPLEMENTATION
    #define "pf.h"

SECTIONS
    PANICS AND ASSERTIONS
        A small set of functions (or rather macros) for panics (panic, panicf)
        and assertions (assert, assertf) and aborting the program (fatal).

    ALLOCATORS
        An allocator interface, functions to interact with allocators
        implementing the interface, and some readymade implementations.

    STRINGS AND STRING VIEWS
        A string type (str) for strings with length information as well as a
        string view type (strv) to easily slice the string, along
        with various functions for working with them.

    LISTS
        Lists / dynamic arrays for arbitrary types.

HOW TO USE
    See usage.c in the root of the repository for some simple examples.

SHORT NAMES
    For convenience of use, the library exposes shorter declarations not
    qualified with the "pf_" prefix. If these conflict with the names in your
    code, define this flag before including the header:

    #define PF_NO_SHORT_NAMES
*/

#ifndef PF_H
#define PF_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#error C11 support required!
#endif

#ifndef PF_NO_SHORT_NAMES
#undef panic
#define panic pf_panic
#undef panicf
#define panicf pf_panicf
#undef fatal
#define fatal pf_fatal
#undef assert
#define assert pf_assert
#undef assertf
#define assertf pf_assertf
#undef todo
#define todo pf_todo
#undef is_power_of_two
#define is_power_of_two pf_is_power_of_two
#undef DEFAULT_ALIGNMENT
#define DEFAULT_ALIGNMENT PF_DEFAULT_ALIGNMENT
#undef align_up_s
#define align_up_s pf_align_up_s
#undef align_up_u
#define align_up_u pf_align_up_u
#undef allocator_alloc_fn
#define allocator_alloc_fn pf_allocator_alloc_fn
#undef allocator_remap_fn
#define allocator_remap_fn pf_allocator_remap_fn
#undef allocator_free_fn
#define allocator_free_fn pf_allocator_free_fn
#undef allocator_free_all_fn
#define allocator_free_all_fn pf_allocator_free_all_fn
#undef allocator_vtable_t
#define allocator_vtable_t pf_allocator_vtable_t
#undef allocator_t
#define allocator_t pf_allocator_t
#undef mem_alloc
#define mem_alloc pf_mem_alloc
#undef mem_remap
#define mem_remap pf_mem_remap
#undef mem_free
#define mem_free pf_mem_free
#undef mem_free_all
#define mem_free_all pf_mem_free_all
#undef malloc_allocator
#define malloc_allocator pf_malloc_allocator
#undef default_allocator
#define default_allocator pf_default_allocator
#undef arena_t
#define arena_t pf_arena_t
#undef arena_init
#define arena_init pf_arena_init
#undef arena_allocator
#define arena_allocator pf_arena_allocator
#undef str_t
#define str_t pf_str_t
#undef strv_t
#define strv_t pf_strv_t
#undef rune_decode
#define rune_decode pf_rune_decode
#undef rune_encode
#define rune_encode pf_rune_encode
#undef RUNETOASCII
#define RUNETOASCII PF_RUNETOASCII
#undef STRTOVIEW
#define STRTOVIEW PF_STRTOVIEW
#undef strv_new_from_cstring
#define strv_new_from_cstring pf_strv_new_from_cstring
#undef STRLIT
#define STRLIT PF_STRLIT
#undef str_new_alloc
#define str_new_alloc pf_str_new_alloc
#undef str_new_alloc_formatted
#define str_new_alloc_formatted pf_str_new_alloc_formatted
#undef str_free
#define str_free pf_str_free
#undef str_to_cstring
#define str_to_cstring pf_str_to_cstring
#undef strv_to_cstring_unsafe
#define strv_to_cstring_unsafe pf_strv_to_cstring_unsafe
#undef strv_to_cstring_alloc
#define strv_to_cstring_alloc pf_strv_to_cstring_alloc
#undef str_grow
#define str_grow pf_str_grow
#undef str_concat
#define str_concat pf_str_concat
#undef str_equal
#define str_equal pf_str_equal
#undef str_slice
#define str_slice pf_str_slice
#undef str_trim_left
#define str_trim_left pf_str_trim_left
#undef str_trim_right
#define str_trim_right pf_str_trim_right
#undef str_trim
#define str_trim pf_str_trim
#undef str_index
#define str_index pf_str_index
#undef str_index_byte
#define str_index_byte pf_str_index_byte
#undef str_print
#define str_print pf_str_print
#undef str_iterator_t
#define str_iterator_t pf_str_iterator_t
#undef str_iterator_new
#define str_iterator_new pf_str_iterator_new
#undef str_iterator_next
#define str_iterator_next pf_str_iterator_next
#undef str_iterator_is_done
#define str_iterator_is_done pf_str_iterator_is_done
#undef LIST_INITIAL_CAP
#define LIST_INITIAL_CAP PF_LIST_INITIAL_CAP
#undef list_t
#define list_t pf_list_t
#undef list_init
#define list_init pf_list_init
#undef list_len
#define list_len pf_list_len
#undef list_cap
#define list_cap pf_list_cap
#undef list_append
#define list_append pf_list_append
#undef list_get
#define list_get pf_list_get
#undef list_pop
#define list_pop pf_list_pop
#undef list_free
#define list_free pf_list_free
#endif  // PF_NO_SHORT_NAMES

/// ----------------
/// Headers
/// ----------------

#include <stdalign.h>  // For memory stuff
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>  // size_t
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>  // For panic_impl
#include <string.h>

/// ----------------
/// Panics and assertions
/// ----------------

// Print an integer to `stream`.
static inline void fputi(int n, FILE *stream)
{
    char buf[12];
    char *p = buf + sizeof buf;
    unsigned u = n < 0 ? -(unsigned) n : (unsigned) n;

    do {
        *--p = '0' + u % 10;
    } while (u /= 10);
    if (n < 0)
        *--p = '-';

    size_t len = buf + sizeof buf - p;
    for (size_t i = 0; i < len; i++)
        fputc(p[i], stream);
}

// End the program with a formatted error message.
// If do_abort is true, the program aborts instead of exiting and avoids
// calling any formatted printing functions, only writing the prefix and
// format string.
noreturn static inline void panic_impl(const char *prefix, const char *file,
                                       const char *function_name, int line,
                                       bool do_abort, const char *fmt, ...)
{
    if (do_abort) {
        fputs(prefix, stderr);
        fputc(':', stderr);
        fputc(' ', stderr);
        fputs(fmt, stderr);
        fputc('\n', stderr);
        fputs("    at line ", stderr);
        fputi(line, stderr);
        fputs(" in file ", stderr);
        fputs(file, stderr);
        fputc('\n', stderr);
        fputs("    in function ", stderr);
        fputs(function_name, stderr);
        fputc('\n', stderr);
        abort();
    } else {
        va_list list;
        va_start(list, fmt);
        fprintf(stderr, "%s: ", prefix);
        vfprintf(stderr, fmt, list);
        va_end(list);
        putc('\n', stderr);
        fprintf(stderr,
                "    at line %d in file %s\n"
                "    in function %s\n",
                line, file, function_name);
        exit(1);
    }
}

// End the program with an error message.
#define pf_panic(message) \
    panic_impl("panic", __FILE__, __func__, __LINE__, false, (message))

// End the program with a formatted error message.
#define pf_panicf(fmt, ...) \
    panic_impl("panic", __FILE__, __func__, __LINE__, false, (fmt), __VA_ARGS__)

// Abort the program with an error message.
#define pf_fatal(message) \
    panic_impl("fatal", __FILE__, __func__, __LINE__, true, (message))

// Assert the truth of a condition, panicking with the given error message if it
// is false.
#define pf_assert(expr, message)                                           \
    do {                                                                   \
        if (!(expr))                                                       \
            panic_impl("assertion `" #expr "` failed", __FILE__, __func__, \
                       __LINE__, false, (message));                        \
    } while (0)

// Same as pf_assert but prints a formatted error message on failure.
#define pf_assertf(expr, fmt, ...)                                         \
    do {                                                                   \
        if (!(expr))                                                       \
            panic_impl("assertion `" #expr "` failed", __FILE__, __func__, \
                       __LINE__, false, (fmt), __VA_ARGS__);               \
    } while (0)

// End the program with a formatted error message, indicating that an
// implementation is not yet done.
#define pf_todo(message) \
    panic_impl("todo", __FILE__, __func__, __LINE__, false, (message))

/// ----------------
/// Memory stuff
/// ----------------

#define BYTES(n) (n)
#define KILOBYTES(n) ((n) << 10)
#define MEGABYTES(n) ((n) << 20)
#define GIGABYTES(n) ((uint64_t) (n) << 30)
#define TERABYTES(n) ((uint64_t) (n) << 40)

static inline bool pf_is_power_of_two(size_t x)
{ return (x & (x - 1)) == 0; }

#define PF_DEFAULT_ALIGNMENT alignof(max_align_t)

static inline size_t pf_align_up_s(size_t size, size_t alignment)
{
    pf_assert(pf_is_power_of_two(alignment),
              "alignment must be a power of two");
    size_t s = size;
    size_t a = alignment;
    size_t modulo = s & (a - 1);
    if (modulo != 0) {
        s += a - modulo;
    }
    return s;
}

static inline uintptr_t pf_align_up_u(uintptr_t ptr, size_t alignment)
{
    pf_assert(pf_is_power_of_two(alignment),
              "alignment must be a power of two");
    uintptr_t p = ptr;
    uintptr_t a = (uintptr_t) alignment;
    uintptr_t modulo = p & (a - 1);
    if (modulo != 0) {
        p += a - modulo;
    }
    return p;
}

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

// A memory arena. It is simply a byte buffer, with the current (and previous)
// offset tracked.
typedef struct {
    uint8_t *buf;
    size_t size;
    size_t curr;
    size_t prev;
} pf_arena_t;

// Initialize the arena allocator `arena` with a backing buffer.
// The backing buffer must remain alive as long as the arena is alive.
void pf_arena_init(pf_arena_t *arena, void *backing_buf,
                   size_t backing_buf_len);

// The arena allocator.
// All allocations made through it are stored in the given arena, and
// deallocated all at once via `free_all`.
// Calling `free` with this allocator is a no-op.
pf_allocator_t pf_arena_allocator(pf_arena_t *arena);

/// ----------------
/// Strings and string views
/// ----------------

// A growable string encoded in UTF-8.
typedef struct {
    char *_s;    // Pointer to the start of the string.
    size_t len;  // Length of the string in bytes.
} pf_str_t;

// A slice, or non-owning view, of a pf_str_t.
// It is almost the same struct as pf_str_t, but defined separately so that
// there is a clear distinction between the two. The one difference is that the
// internal string pointer is const, because string views are immutable.
typedef struct {
    const char *_s;
    size_t len;
} pf_strv_t;

// A Unicode codepoint.
typedef uint32_t rune_t;

// Decode a single codepoint from a UTF-8 encoded string.
// Returns the codepoint, and stores its byte size in `len`.
// Assumes the provided string is valid UTF-8.
// Function definition adapted from the one shown in Jimmy Lefevre's talk at
// Better Software Conference 2026.
rune_t pf_rune_decode(char *s, size_t *len);

// Encode a codepoint to UTF-8.
// Returns byte size of the encoded codepoint, and the encoded bytes in `out`.
// If `cp` is not a valid Unicode codepoint, -1 is returned.
int pf_rune_encode(rune_t cp, char out[4]);

// Returns the ASCII value for the given rune_t if it is in range. Otherwise
// it returns -1.
// If the return value is not -1, it can be safely casted to `char`.
static inline int PF_RUNETOASCII(rune_t r)
{ return (r < 128) ? (int) r : -1; }

// Convert a pf_str_t to a pf_strv_t.
static inline pf_strv_t PF_STRTOVIEW(pf_str_t s)
{ return (pf_strv_t) { ._s = s._s, .len = s.len }; }

// Create a new pf_strv_t of size `len` from a C-string `buf`,
// where `buf` is a buffer managed by the caller that MUST to have a size of at
// least `len + 1` characters AND have `buf[len]` equal to '\0'.
pf_strv_t pf_strv_new_from_cstring(const char *buf, int len);

// Create a pf_strv_t from a string literal.
#define PF_STRLIT(lit) pf_strv_new_from_cstring((lit), sizeof((lit)) - 1)

// Create a new pf_str_t of size `len`.
// Allocates on the heap! Call pf_free to free the result.
pf_str_t pf_str_new_alloc(size_t len);

// Create a new formatted pf_str_t.
// Allocates on the heap! Call pf_str_free to free the result.
pf_str_t pf_str_new_alloc_formatted(const char *fmt, ...);

// Free a heap-allocated pf_str_t.
void pf_str_free(pf_str_t s);

// Get the underlying C-string for the given pf_str_t.
const char *pf_str_to_cstring(pf_str_t s);

// Return the underlying C-string for the given pf_strv_t.
// Warning! This is unsafe because the underlying string may not contain a
// null terminator!
const char *pf_strv_to_cstring_unsafe(pf_strv_t s);

// Create a properly null-terminated C-string for the given pf_strv_t.
// Allocates on the heap! Call your relevant allocator's free function to free
// the result.
const char *pf_strv_to_cstring_alloc(pf_strv_t s);

void _pf_str_grow_with_strv(pf_str_t *a, pf_strv_t b);
void _pf_str_grow_with_str(pf_str_t *a, pf_str_t b);

// Grow a pf_str_t by concatenating another pf_str_t or pf_strv_t to it,
// in-place. Allocates on the heap! Reallocates `a` to fit `b`.
#define pf_str_grow(a, b)                \
    _Generic((b),                        \
        pf_str_t: _pf_str_grow_with_str, \
        pf_strv_t: _pf_str_grow_with_strv)((a), (b))

pf_str_t _pf_strv_concat(pf_strv_t a, pf_strv_t b);
pf_str_t _pf_str_concat(pf_str_t a, pf_str_t b);

// Concatenate two pf_strs or to pf_strvs with each other to produce a new
// pf_str_t.
// Allocates on the heap! Call pf_free to free the result.
#define pf_str_concat(a, b)                                                  \
    _Generic((a), pf_str_t: _pf_str_concat, pf_strv_t: _pf_strv_concat)((a), \
                                                                        (b))

bool _pf_strv_equal(pf_strv_t a, pf_strv_t b);
bool _pf_str_equal(pf_str_t a, pf_str_t b);

// Determines whether two pf_strs or pf_strvs are equal in content.
#define pf_str_equal(a, b) \
    _Generic((a), pf_str_t: _pf_str_equal, pf_strv_t: _pf_strv_equal)((a), (b))

pf_strv_t _pf_strv_slice(pf_strv_t sv, size_t start, size_t end);
pf_strv_t _pf_str_slice(pf_str_t s, size_t start, size_t end);

// Given the pf_str_t or pf_strv_t `s`, return a pf_strv_t sliced
// from index `start` to `end`, exclusive at `end`.
// The slice indices are byte indices.
// If `start` or `end` is too small or too large in relation to the length
// of `s`, or if `start` is larger than `end`, an error message
// is printed and the program exits.
#define pf_str_slice(s, start, end)                                    \
    _Generic((s), pf_str_t: _pf_str_slice, pf_strv_t: _pf_strv_slice)( \
        (s), (start), (end))

pf_strv_t _pf_strv_trim_left(pf_strv_t sv, const char *cutset);
pf_strv_t _pf_str_trim_left(pf_str_t s, const char *cutset);

// Create a pf_strv_t from the given `s` with any bytes that match the ones
// in `cutset` removed from the left end.
// `s` can be a pf_str_t or pf_strv_t.
#define pf_str_trim_left(s, cutset)                                            \
    _Generic((s), pf_str_t: _pf_str_trim_left, pf_strv_t: _pf_strv_trim_left)( \
        (s), (cutset))

pf_strv_t _pf_strv_trim_right(pf_strv_t sv, const char *cutset);
pf_strv_t _pf_str_trim_right(pf_str_t s, const char *cutset);

// Create a pf_strv_t from the given `s` with any bytes that match the ones
// in `cutset` removed from the right end.
// `s` can be a pf_str_t or pf_strv_t.
#define pf_str_trim_right(s, cutset)  \
    _Generic((s),                     \
        pf_str_t: _pf_str_trim_right, \
        pf_strv_t: _pf_strv_trim_right)((s), (cutset))

pf_strv_t _pf_strv_trim(pf_strv_t sv, const char *cutset);
pf_strv_t _pf_str_trim(pf_str_t s, const char *cutset);

// Create a pf_strv_t from the given `s` with any bytes that match the ones
// in `cutset` removed from the left and right ends.
// `s` can be a pf_str_t or pf_strv_t.
#define pf_str_trim(s, cutset)                                           \
    _Generic((s), pf_str_t: _pf_str_trim, pf_strv_t: _pf_strv_trim)((s), \
                                                                    (cutset))

rune_t _pf_strv_index(pf_strv_t sv, size_t idx);
rune_t _pf_str_index(pf_str_t s, size_t idx);

// Index the codepoint `idx` in `s` and return it as a rune_t.
// `s` may be a pf_str_t or pf_strv_t.
// This is an O(n) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pf_str_index(s, idx)                                               \
    _Generic((s), pf_str_t: _pf_str_index, pf_strv_t: _pf_strv_index)((s), \
                                                                      (idx))

char _pf_strv_index_byte(pf_strv_t sv, size_t idx);
char _pf_str_index_byte(pf_str_t s, size_t idx);

// Index the byte `idx` in `s` and return it as a char.
// `s` may be a pf_str_t or pf_strv_t.
// This is an O(1) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pf_str_index_byte(s, idx)     \
    _Generic((s),                     \
        pf_str_t: _pf_str_index_byte, \
        pf_strv_t: _pf_strv_index_byte)((s), (idx))

void _pf_strv_print(pf_strv_t sv);
void _pf_str_print(pf_str_t s);

// Prints a pf_str_t or pf_strv_t.
// You could print using standard C functions like printf; but pf_print
// handles string slices as well as runes.
#define pf_str_print(s) \
    _Generic((s), pf_str_t: _pf_str_print, pf_strv_t: _pf_strv_print)((s))

typedef enum {
    _pf_str_iterator_iterating,
    _pf_str_iterator_done,
} _pf_str_iterator_state_t;  // State of the iterator.

// An iterator over codepoints in a pf_strv_t.
typedef struct {
    pf_strv_t _strv;                  // String view being iterated over.
    _pf_str_iterator_state_t _state;  // State of the iterator.
} pf_str_iterator_t;

pf_str_iterator_t _pf_strv_iterator_new(pf_strv_t sv);
pf_str_iterator_t _pf_str_iterator_new(pf_str_t sv);

// Create an iterator over codepoints of a pf_str_t or pf_strv_t.
#define pf_str_iterator_new(s)          \
    _Generic((s),                       \
        pf_str_t: _pf_str_iterator_new, \
        pf_strv_t: _pf_strv_iterator_new)((s))

// Decode and get the next codepoint from `it` as a rune_t.
// If `len` is not NULL, the byte size of the codepoint is stored in it.
// If the iterator is done, it returns the 0 (the null character) and stores
// 0 in `len` (if it is not NULL). It is recommended to use
// pf_iterator_is_done to check if the iterator is done, though.
rune_t pf_str_iterator_next(pf_str_iterator_t *it, size_t *len);

// Checks if the iterator `it` is exhausted.
bool pf_str_iterator_is_done(pf_str_iterator_t it);

/// ----------------
/// Lists
/// ----------------

// Starting size of a pf_list_t after the first append.
#define PF_LIST_INITIAL_CAP 8

#define _PF_LIST_HEADER_SIZE \
    pf_align_up_s(2 * sizeof(size_t), PF_DEFAULT_ALIGNMENT)

#define _PF_LIST_HEADER(l) ((size_t *) ((uint8_t *) (l) - _PF_LIST_HEADER_SIZE))

// Type of a growable dynamic array for a certain type T.
// T can be any type, except for an over-aligned type; they are not supported.
// Pointers to elements within a list are NOT guaranteed to stay valid.
#define pf_list_t(T) T *

// Create a new empty pf_list_t.
#define pf_list_init(T) (T *) NULL

// Get the number of items in the given pf_list_t.
#define pf_list_len(l) ((l) == NULL ? 0 : (_PF_LIST_HEADER(l))[0])

// Get the number of items the given pf_list_t can currently hold.
#define pf_list_cap(l) ((l) == NULL ? 0 : (_PF_LIST_HEADER(l))[1])

// Add the provided item to the end of the list `l`.
// `l` MUST be a variable that refers to a pf_list_t, unexpected bad things will
// happen if it is not!
// Allocates on the heap if the provided list is empty or if it doesn't have
// enough capacity for the new item.
#define pf_list_append(l, item)                                            \
    do {                                                                   \
        if ((l) == NULL) {                                                 \
            void *buf = malloc(_PF_LIST_HEADER_SIZE +                      \
                               (PF_LIST_INITIAL_CAP * sizeof(*(l))));      \
            if (!buf) {                                                    \
                pf_fatal("out of memory!");                                \
            }                                                              \
            size_t *header = buf;                                          \
            header[0] = 0;                                                 \
            header[1] = PF_LIST_INITIAL_CAP;                               \
            (l) = (void *) ((uint8_t *) buf + _PF_LIST_HEADER_SIZE);       \
        }                                                                  \
        size_t *header = _PF_LIST_HEADER(l);                               \
        size_t len = header[0];                                            \
        size_t cap = header[1];                                            \
        if (len >= cap) {                                                  \
            if (cap > (SIZE_MAX / 2)) {                                    \
                pf_panicf(                                                 \
                    "attempt to grow a pf_list_t over capacity limit of "  \
                    "%zu items",                                           \
                    SIZE_MAX);                                             \
            }                                                              \
            size_t newcap = cap * 2;                                       \
            if (newcap > (SIZE_MAX / sizeof(*(l)))) {                      \
                pf_panicf(                                                 \
                    "attempt to grow a pf_list_t over byte size limit of " \
                    "%zu bytes",                                           \
                    SIZE_MAX);                                             \
            }                                                              \
            void *buf = realloc(header, _PF_LIST_HEADER_SIZE +             \
                                            (newcap * sizeof(*(l))));      \
            if (!buf) {                                                    \
                pf_fatal("out of memory!");                                \
            }                                                              \
            header = buf;                                                  \
            header[1] = newcap;                                            \
            (l) = (void *) ((uint8_t *) buf + _PF_LIST_HEADER_SIZE);       \
        }                                                                  \
        l[len] = (item);                                                   \
        header[0] = len + 1;                                               \
    } while (0)

// Get an element from the list `l` at index `i`.
// It is essentially the same as l[i], however it is bounds checked and will
// therefore panic if the given index is outside the bounds of the list.
// `l` MUST be a variable that refers to a pf_list_t, unexpected bad things will
// happen if it is not!
#define pf_list_get(l, i)                                                    \
    (i >= 0 && i < pf_list_len(l)                                            \
         ? (l)[i]                                                            \
         : (pf_panicf(                                                       \
                "attempt to index a pf_list_t of length %zu with index %zu", \
                pf_list_len(l), i),                                          \
            (l)[0]))

// Remove the last element from the list and return it.
// `l` MUST be a variable that refers to a pf_list_t, unexpected bad things will
// happen if it is not!
// If the given list is of size 0, panics.
#define pf_list_pop(l)                  \
    (pf_list_len(l) > 0                 \
         ? (l)[--_PF_LIST_HEADER(l)[0]] \
         : (pf_panic("attempt to pop a pf_list_t that is empty"), (l)[0]))

// Free the heap memory of the provided pf_list_t.
// `l` MUST be a variable that refers to a pf_list_t, unexpected bad things will
// happen if it is not!
// It is safe to pass an empty pf_list_t here.
#define pf_list_free(l)                                   \
    do {                                                  \
        if ((l) != NULL) {                                \
            free((uint8_t *) (l) - _PF_LIST_HEADER_SIZE); \
            (l) = NULL;                                   \
        }                                                 \
    } while (0)

#endif  // PF_H

/// ----------------
/// Implementation
/// ----------------

#ifdef PF_IMPLEMENTATION
#ifndef _PF_IMPLEMENTED
#define _PF_IMPLEMENTED

/// ----------------
/// Allocator interface
/// ----------------

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

void pf_arena_init(pf_arena_t *arena, void *backing_buf, size_t backing_buf_len)
{
    *arena = (pf_arena_t) {
        .buf = backing_buf,
        .size = backing_buf_len,
        .curr = 0,
        .prev = 0,
    };
}

static void *pf_arena_allocator_alloc_align(pf_arena_t *arena, size_t size,
                                            size_t alignment)
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
    pf_arena_t *arena = (pf_arena_t *) ctx;
    return pf_arena_allocator_alloc_align(arena, len, PF_DEFAULT_ALIGNMENT);
}

// Doesn't do anything, just here for completeness.
static void pf_arena_allocator_free(void *ctx, void *ptr)
{
    (void) ctx;
    (void) ptr;
}

static void *pf_arena_allocator_remap_align(pf_arena_t *arena, void *ptr,
                                            size_t old_size, size_t new_size,
                                            size_t alignment)
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
    pf_arena_t *arena = (pf_arena_t *) ctx;
    return pf_arena_allocator_remap_align(arena, ptr, old_len, new_len,
                                          PF_DEFAULT_ALIGNMENT);
}

static void pf_arena_allocator_free_all(void *ctx)
{
    pf_arena_t *arena = (pf_arena_t *) ctx;
    arena->curr = 0;
    arena->prev = 0;
}

static const pf_allocator_vtable_t pf_arena_allocator_vtable = {
    .alloc = pf_arena_allocator_alloc,
    .remap = pf_arena_allocator_remap,
    .free = pf_arena_allocator_free,
    .free_all = pf_arena_allocator_free_all,
};

pf_allocator_t pf_arena_allocator(pf_arena_t *arena)
{
    return (pf_allocator_t) {
        .ctx = arena,
        .vtable = &pf_arena_allocator_vtable,
    };
}

/// ----------------
/// Strings and string views
/// ----------------

rune_t pf_rune_decode(char *s, size_t *len)
{
    rune_t cp;
    switch (*s & 0xf0) {
    case 0xf0:
        cp = *s & 7;
        *len = 4;
        break;
    case 0xe0:
        cp = *s & 0xf;
        *len = 3;
        break;
    case 0xd0:
    case 0xc0:
        cp = *s & 0x1f;
        *len = 2;
        break;
    default:
        cp = *s;
        *len = 1;
        break;
    }

    for (int i = *len; --i;) {
        cp = (cp << 6) | (*(++s) & 0x3f);
    }
    return cp;
}

int pf_rune_encode(rune_t cp, char out[4])
{
    int len;
    if (cp <= 0x7f) {
        out[0] = cp;
        len = 1;
    } else if (cp <= 0x7ff) {
        out[0] = 0xc0 | (cp >> 6);
        out[1] = 0x80 | (cp & 0x3f);
        len = 2;
    } else if (cp <= 0xd7ff || (cp >= 0xe000 && cp <= 0xffff)) {
        out[0] = 0xe0 | (cp >> 12);
        out[1] = 0x80 | ((cp >> 6) & 0x3f);
        out[2] = 0x80 | (cp & 0x3f);
        len = 3;
    } else if (cp >= 0x10000 && cp <= 0x10ffff) {
        out[0] = 0xf0 | (cp >> 18);
        out[1] = 0x80 | ((cp >> 12) & 0x3f);
        out[2] = 0x80 | ((cp >> 6) & 0x3f);
        out[3] = 0x80 | (cp & 0x3f);
        len = 4;
    } else {
        len = -1;  // invalid codepoint
    }
    return len;
}

pf_strv_t pf_strv_new_from_cstring(const char *buf, int len)
{
    return (pf_strv_t) {
        ._s = buf,
        .len = len,
    };
}

pf_str_t pf_str_new_alloc(size_t len)
{
    char *buf = malloc(len + 1);  // for null terminator
    if (!buf) {
        pf_fatal("out of memory!");
    }
    buf[len] = '\0';
    return (pf_str_t) {
        ._s = buf,
        .len = len,
    };
}

pf_str_t pf_str_new_alloc_formatted(const char *fmt, ...)
{
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        va_end(args2);
        pf_panic("failed to format string\n");
    }

    char *buf = malloc(len + 1);
    if (!buf) {
        va_end(args2);
        pf_fatal("out of memory!");
    }

    vsnprintf(buf, len + 1, fmt, args2);
    va_end(args2);

    return (pf_str_t) { ._s = buf, .len = len };
}

void pf_str_free(pf_str_t s)
{
    if (s._s == NULL)
        return;
    free(s._s);
    s._s = NULL;
}

const char *pf_str_to_cstring(pf_str_t s)
{ return s._s; }

const char *pf_strv_to_cstring_unsafe(pf_strv_t s)
{ return s._s; }

const char *pf_strv_to_cstring_alloc(pf_strv_t s)
{
    char *buf = malloc(s.len + 1);
    if (!buf) {
        pf_fatal("out of memory!");
    }

    memcpy(buf, s._s, s.len);
    buf[s.len] = '\0';
    return buf;
}

// TODO: fix use after free when this is called with `b` aliasing `a`
void _pf_str_grow_with_strv(pf_str_t *a, pf_strv_t b)
{
    if (a->len > SIZE_MAX - b.len - 1) {
        pf_panicf("attempt to grow a pf_str_t over size limit of %zu bytes",
                  SIZE_MAX);
    }

    size_t newlen = a->len + b.len;
    char *new = realloc(a->_s, newlen + 1);
    if (!new) {
        pf_fatal("out of memory!");
    }
    a->_s = new;

    memcpy(a->_s + a->len, b._s, b.len);
    a->_s[newlen] = '\0';
    a->len = newlen;
}

void _pf_str_grow_with_str(pf_str_t *a, pf_str_t b)
{ _pf_str_grow_with_strv(a, PF_STRTOVIEW(b)); }

pf_str_t _pf_strv_concat(pf_strv_t a, pf_strv_t b)
{
    if (a.len > SIZE_MAX - b.len - 1) {
        pf_panicf("attempt to create a concatenated pf_str_t over "
                  "size limit of %zu bytes",
                  SIZE_MAX);
    }

    size_t len = a.len + b.len;
    char *buf = malloc(len + 1);
    if (!buf) {
        pf_fatal("out of memory!");
    }

    memcpy(buf, a._s, a.len);
    memcpy(buf + a.len, b._s, b.len);
    buf[len] = '\0';
    return (pf_str_t) { ._s = buf, .len = len };
}

pf_str_t _pf_str_concat(pf_str_t a, pf_str_t b)
{ return _pf_strv_concat(PF_STRTOVIEW(a), PF_STRTOVIEW(b)); }

bool _pf_strv_equal(pf_strv_t a, pf_strv_t b)
{
    if (a.len != b.len)
        return false;

    for (size_t i = 0; i < a.len; i++)
        if (a._s[i] != b._s[i])
            return false;

    return true;
}

bool _pf_str_equal(pf_str_t a, pf_str_t b)
{ return _pf_strv_equal(PF_STRTOVIEW(a), PF_STRTOVIEW(b)); }

pf_strv_t _pf_strv_slice(pf_strv_t sv, size_t start, size_t end)
{
    if (start > end) {
        pf_panicf("starting slice index %zu is larger than ending "
                  "index %zu",
                  start, end);
    }

    if (start > sv.len) {
        pf_panicf("invalid starting slice index %zu for s of length "
                  "%zu",
                  start, sv.len);
    }

    if (end > sv.len) {
        pf_panicf("invalid ending slice index %zu for s of length %zu", end,
                  sv.len);
    }

    return (pf_strv_t) {
        ._s = sv._s + start,
        .len = end - start,
    };
}

pf_strv_t _pf_str_slice(pf_str_t s, size_t start, size_t end)
{ return _pf_strv_slice(PF_STRTOVIEW(s), start, end); }

rune_t _pf_strv_index(pf_strv_t sv, size_t idx)
{
    const char *p = pf_strv_to_cstring_unsafe(sv);
    size_t cps = 0;
    size_t bytes = 0;
    while (bytes < sv.len) {
        size_t len;
        rune_t cp = pf_rune_decode((char *) p, &len);
        if (cps == idx)
            return cp;

        p += len;
        bytes += len;
        cps += 1;
    }

    pf_panicf("invalid index %zu for s with %d codepoints", idx, cps);
}

rune_t _pf_str_index(pf_str_t s, size_t idx)
{ return _pf_strv_index(PF_STRTOVIEW(s), idx); }

char _pf_strv_index_byte(pf_strv_t sv, size_t idx)
{
    if (idx >= sv.len) {
        pf_panicf("invalid byte index %zu for s of length %zu", idx, sv.len);
    }

    return sv._s[idx];
}

char _pf_str_index_byte(pf_str_t s, size_t idx)
{ return _pf_strv_index_byte(PF_STRTOVIEW(s), idx); }

static bool is_in_cutset(const char *cutset, char c)
{
    const char *p = cutset;
    while (*p) {
        if (c == *p) {
            return true;
        }
        p++;
    }
    return false;
}

pf_strv_t _pf_strv_trim_left(pf_strv_t sv, const char *cutset)
{
    if (sv.len == 0 || (strcmp(cutset, "") == 0))
        return sv;

    pf_strv_t res = sv;
    size_t i = 0;
    while (i < sv.len) {
        char b = pf_str_index_byte(sv, i);
        if (!is_in_cutset(cutset, b))
            break;
        i++;
    }

    res = pf_str_slice(res, i, sv.len);
    return res;
}

pf_strv_t _pf_str_trim_left(pf_str_t s, const char *cutset)
{ return _pf_strv_trim_left(PF_STRTOVIEW(s), cutset); }

pf_strv_t _pf_strv_trim_right(pf_strv_t sv, const char *cutset)
{
    if (sv.len == 0 || (strcmp(cutset, "") == 0))
        return sv;

    pf_strv_t res = sv;
    int i = sv.len - 1;
    while (i >= 0) {
        char b = pf_str_index_byte(sv, i);
        if (!is_in_cutset(cutset, b))
            break;
        i--;
    }

    res = pf_str_slice(res, 0, i + 1);
    return res;
}

pf_strv_t _pf_str_trim_right(pf_str_t s, const char *cutset)
{ return _pf_strv_trim_right(PF_STRTOVIEW(s), cutset); }

pf_strv_t _pf_strv_trim(pf_strv_t sv, const char *cutset)
{
    pf_strv_t left = _pf_strv_trim_left(sv, cutset);
    return _pf_strv_trim_right(left, cutset);
}

pf_strv_t _pf_str_trim(pf_str_t s, const char *cutset)
{ return _pf_strv_trim(PF_STRTOVIEW(s), cutset); }

void _pf_strv_print(pf_strv_t sv)
{
    for (size_t i = 0; i < sv.len; i++)
        putchar(sv._s[i]);
}

void _pf_str_print(pf_str_t s)
{ _pf_strv_print(PF_STRTOVIEW(s)); }

pf_str_iterator_t _pf_strv_iterator_new(pf_strv_t sv)
{
    _pf_str_iterator_state_t state = _pf_str_iterator_iterating;
    if (sv.len == 0)
        state = _pf_str_iterator_done;
    return (pf_str_iterator_t) { ._strv = sv, ._state = state };
}

pf_str_iterator_t _pf_str_iterator_new(pf_str_t s)
{ return _pf_strv_iterator_new(PF_STRTOVIEW(s)); }

rune_t pf_str_iterator_next(pf_str_iterator_t *it, size_t *len)
{
    rune_t cp;
    size_t cplen;

    if (it->_state == _pf_str_iterator_iterating) {
        pf_strv_t sv = it->_strv;
        const char *p = pf_strv_to_cstring_unsafe(sv);
        cp = pf_rune_decode((char *) p, &cplen);
        if (len != NULL)
            *len = cplen;

        it->_strv = pf_str_slice(it->_strv, cplen, it->_strv.len);
        if (it->_strv.len <= 0) {
            it->_state = _pf_str_iterator_done;
        }
    } else {
        cp = 0;
        if (len != NULL)
            *len = 0;
    }

    return (rune_t) cp;
}

bool pf_str_iterator_is_done(pf_str_iterator_t it)
{ return it._state == _pf_str_iterator_done; }

#endif  // _PF_IMPLEMENTED
#endif  // PF_IMPLEMENTATION
