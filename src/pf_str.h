#ifndef PF_STR_H
#define PF_STR_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#error C11 support required
#endif

#ifndef PF_STR_NO_SHORT_NAMES
#define str pfs_str
#define strv pfs_strv
#define rune pfs_rune

#define RUNETOASCII PFS_RUNETOASCII
#define CSTRING PFS_CSTRING
#define STRTOVIEW PFS_STRTOVIEW

#define strv_new_from_cstring pfs_strv_new_from_cstring
#define STRLIT PFS_STRLIT

#define str_new_alloc pfs_str_new_alloc
#define str_new_alloc_formatted pfs_str_new_alloc_formatted

#define str_free pfs_str_free

#define str_grow pfs_str_grow
#define str_concat pfs_str_concat

#define str_equal pfs_str_equal

#define str_slice pfs_str_slice
#define str_trim_left pfs_str_trim_left
#define str_trim_right pfs_str_trim_right
#define str_trim pfs_str_trim

#define str_index pfs_str_index
#define str_index_byte pfs_str_index_byte

#define str_print pfs_str_print
#define rune_print pfs_rune_print

#define str_iterator pfs_str_iterator
#define str_iterator_new pfs_str_iterator_new
#define str_iterator_next pfs_str_iterator_next
#define str_iterator_is_done pfs_str_iterator_is_done
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A growable string encoded in UTF-8.
typedef struct {
    char *_s;    // Pointer to the start of the string.
    size_t len;  // Length of the string in bytes.
} pfs_str;

// A slice, or non-owning view, of a pfs_str.
// It is almost the same struct as pfs_str, but defined separately so that there
// is a clear distinction between the two.
// The one difference is that the internal string pointer is const, because
// string views are immutable.
typedef struct {
    const char *_s;
    size_t len;
} pfs_strv;

// A Unicode codepoint.
typedef uint32_t pfs_rune;

// Returns the ASCII value for the given pfs_rune if it is in range. Otherwise
// it returns -1.
// If the return value is not -1, it can be safely casted to `char`.
static inline int PFS_RUNETOASCII(pfs_rune r)
{ return r < 128 ? (r & 127) : -1; }

// Get the C-string representation of a pfs_str.
// Important note: For a pfs_strv, the returned C-string may not necessarily
// be as long as the slice; it will be as long as the backing char * or pfs_str
// instead.
#define PFS_CSTRING(s) (s)._s

// Convert a pfs_str to a pfs_strv.
static inline pfs_strv PFS_STRTOVIEW(pfs_str s)
{ return (pfs_strv) { ._s = s._s, .len = s.len }; }

// Create a new pfs_strv of size `len` from a C-string `buf`,
// where `buf` is a buffer managed by the caller that MUST to have a size of at
// least `len + 1` characters AND have `buf[len]` equal to '\0'.
pfs_strv pfs_strv_new_from_cstring(const char *buf, int len);

// Create a pfs_strv from a string literal.
#define PFS_STRLIT(lit) pfs_strv_new_from_cstring((lit), sizeof((lit)) - 1)

// Create a new pfs_str of size `len`.
// Allocates on the heap! Call pfs_free to free the result.
pfs_str pfs_str_new_alloc(size_t len);

// Create a new formatted pfs_str.
// Allocates on the heap! Call pfs_str_free to free the result.
pfs_str pfs_str_new_alloc_formatted(const char *fmt, ...);

// Free a heap-allocated pfs_str.
void pfs_str_free(pfs_str s);

void _pfs_str_grow_with_strv(pfs_str *a, pfs_strv b);
void _pfs_str_grow_with_str(pfs_str *a, pfs_str b);

// Grow a pfs_str by concatenating another pfs_str or pfs_strv to it, in-place.
// Allocates on the heap! Reallocates `a` to fit `b`.
#define pfs_str_grow(a, b)               \
    _Generic((b),                        \
        pfs_str: _pfs_str_grow_with_str, \
        pfs_strv: _pfs_str_grow_with_strv)((a), (b))

pfs_str _pfs_strv_concat(pfs_strv a, pfs_strv b);
pfs_str _pfs_str_concat(pfs_str a, pfs_str b);

// Concatenate two pfss or to pfs_strvs with each other to produce a new
// pfs_str.
// Allocates on the heap! Call pfs_free to free the result.
#define pfs_str_concat(a, b)                                                 \
    _Generic((a), pfs_str: _pfs_str_concat, pfs_strv: _pfs_strv_concat)((a), \
                                                                        (b))

bool _pfs_strv_equal(pfs_strv a, pfs_strv b);
bool _pfs_str_equal(pfs_str a, pfs_str b);

// Determines whether two pfss or pfs_strvs are equal in content.
#define pfs_str_equal(a, b) \
    _Generic((a), pfs_str: _pfs_str_equal, pfs_strv: _pfs_strv_equal)((a), (b))

pfs_strv _pfs_strv_slice(pfs_strv sv, size_t start, size_t end);
pfs_strv _pfs_str_slice(pfs_str s, size_t start, size_t end);

// Given the pfs_str or pfs_strv `s`, return a pfs_strv sliced
// from index `start` to `end`, exclusive at `end`.
// If `start` or `end` is too small or too large in relation to the length
// of `s`, or if `start` is larger than `end`, an error message
// is printed and the program exits.
#define pfs_str_slice(s, start, end)                                   \
    _Generic((s), pfs_str: _pfs_str_slice, pfs_strv: _pfs_strv_slice)( \
        (s), (start), (end))

pfs_strv _pfs_strv_trim_left(pfs_strv sv, const char *cutset);
pfs_strv _pfs_str_trim_left(pfs_str s, const char *cutset);

// Create a pfs_strv from the given `s` with any bytes that match the ones
// in `cutset` removed from the left end.
// `s` can be a pfs_str or pfs_strv.
#define pfs_str_trim_left(s, cutset)                                           \
    _Generic((s), pfs_str: _pfs_str_trim_left, pfs_strv: _pfs_strv_trim_left)( \
        (s), (cutset))

pfs_strv _pfs_strv_trim_right(pfs_strv sv, const char *cutset);
pfs_strv _pfs_str_trim_right(pfs_str s, const char *cutset);

// Create a pfs_strv from the given `s` with any bytes that match the ones
// in `cutset` removed from the right end.
// `s` can be a pfs_str or pfs_strv.
#define pfs_str_trim_right(s, cutset) \
    _Generic((s),                     \
        pfs_str: _pfs_str_trim_right, \
        pfs_strv: _pfs_strv_trim_right)((s), (cutset))

pfs_strv _pfs_strv_trim(pfs_strv sv, const char *cutset);
pfs_strv _pfs_str_trim(pfs_str s, const char *cutset);

// Create a pfs_strv from the given `s` with any bytes that match the ones
// in `cutset` removed from the left and right ends.
// `s` can be a pfs_str or pfs_strv.
#define pfs_str_trim(s, cutset)                                          \
    _Generic((s), pfs_str: _pfs_str_trim, pfs_strv: _pfs_strv_trim)((s), \
                                                                    (cutset))

pfs_rune _pfs_strv_index(pfs_strv sv, size_t idx);
pfs_rune _pfs_str_index(pfs_str s, size_t idx);

// Index the codepoint `idx` in `s` and return it as a pf_rune.
// `s` may be a pfs_str or pfs_strv.
// This is an O(n) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pfs_str_index(s, idx)                                              \
    _Generic((s), pfs_str: _pfs_str_index, pfs_strv: _pfs_strv_index)((s), \
                                                                      (idx))

char _pfs_strv_index_byte(pfs_strv sv, size_t idx);
char _pfs_str_index_byte(pfs_str s, size_t idx);

// Index the byte `idx` in `s` and return it as a char.
// `s` may be a pfs_str or pfs_strv.
// This is an O(1) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pfs_str_index_byte(s, idx)    \
    _Generic((s),                     \
        pfs_str: _pfs_str_index_byte, \
        pfs_strv: _pfs_strv_index_byte)((s), (idx))

void _pfs_strv_print(pfs_strv sv);
void _pfs_str_print(pfs_str s);

// Prints a pfs_str or pfs_strv.
// You could print using standard C functions like printf; but pfs_print
// handles string slices as well as runes.
#define pfs_str_print(s) \
    _Generic((s), pfs_str: _pfs_str_print, pfs_strv: _pfs_strv_print)((s))

// Decodes and prints a pf_rune.
void pfs_rune_print(pfs_rune r);

typedef enum {
    _pfs_str_iterator_iterating,
    _pfs_str_iterator_done,
} _pfs_str_iterator_state;  // State of the iterator.

// An iterator over codepoints in a pfs_strv.
typedef struct {
    pfs_strv _strv;                  // String view being iterated over.
    _pfs_str_iterator_state _state;  // State of the iterator.
} pfs_str_iterator;

pfs_str_iterator _pfs_strv_iterator_new(pfs_strv sv);
pfs_str_iterator _pfs_str_iterator_new(pfs_str sv);

// Create an iterator over codepoints of a pfs_str or pfs_strv.
#define pfs_str_iterator_new(s)         \
    _Generic((s),                       \
        pfs_str: _pfs_str_iterator_new, \
        pfs_strv: _pfs_strv_iterator_new)((s))

// Decode and get the next codepoint from `it` as a pf_rune.
// If `len` is not NULL, the byte size of the codepoint is stored in it.
// If the iterator is done, it returns the 0 (the null character) and stores
// 0 in `len` (if it is not NULL). It is recommended to use
// pfs_iterator_is_done to check if the iterator is done, though.
pfs_rune pfs_str_iterator_next(pfs_str_iterator *it, size_t *len);

// Checks if the iterator `it` is exhausted.
bool pfs_str_iterator_is_done(pfs_str_iterator it);

#endif
