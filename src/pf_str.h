#ifndef PF_STR_H
#define PF_STR_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#error C11 support required
#endif

#ifndef PF_STR_NO_SHORT_NAMES
#define str pf_str
#define strv pf_strv

#define rune pf_rune
#define rune_decode pf_rune_decode
#define rune_encode pf_rune_encode

#define RUNETOASCII pf_RUNETOASCII
#define CSTRING PF_CSTRING
#define STRTOVIEW pf_STRTOVIEW

#define strv_new_from_cstring pf_strv_new_from_cstring
#define STRLIT PF_STRLIT

#define str_new_alloc pf_str_new_alloc
#define str_new_alloc_formatted pf_str_new_alloc_formatted

#define str_free pf_str_free

#define str_grow pf_str_grow
#define str_concat pf_str_concat

#define str_equal pf_str_equal

#define str_slice pf_str_slice
#define str_trim_left pf_str_trim_left
#define str_trim_right pf_str_trim_right
#define str_trim pf_str_trim

#define str_index pf_str_index
#define str_index_byte pf_str_index_byte

#define str_print pf_str_print

#define str_iterator pf_str_iterator
#define str_iterator_new pf_str_iterator_new
#define str_iterator_next pf_str_iterator_next
#define str_iterator_is_done pf_str_iterator_is_done
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// A growable string encoded in UTF-8.
typedef struct {
    char *_s;    // Pointer to the start of the string.
    size_t len;  // Length of the string in bytes.
} pf_str;

// A slice, or non-owning view, of a pf_str.
// It is almost the same struct as pf_str, but defined separately so that there
// is a clear distinction between the two.
// The one difference is that the internal string pointer is const, because
// string views are immutable.
typedef struct {
    const char *_s;
    size_t len;
} pf_strv;

// A Unicode codepoint.
typedef uint32_t pf_rune;

// Decode a single codepoint from a UTF-8 encoded string.
// Returns the codepoint, and stores its byte size in `len`.
// Assumes the provided string is valid UTF-8.
// Function definition adapted from the one shown in Jimmy Lefevre's talk at
// Better Software Conference 2026.
pf_rune pf_rune_decode(char *s, size_t *len);

// Encode a codepoint to UTF-8.
// Returns byte size of the encoded codepoint, and the encoded bytes in `out`.
// If `cp` is not a valid Unicode codepoint, -1 is returned.
int pf_rune_encode(pf_rune cp, char out[4]);

// Returns the ASCII value for the given pf_rune if it is in range. Otherwise
// it returns -1.
// If the return value is not -1, it can be safely casted to `char`.
static inline int PF_RUNETOASCII(pf_rune r)
{
    char out[4];
    int len = pf_rune_encode(r, out);
    if (len == 1)
        return out[0];
    return -1;
}

// Get the C-string representation of a pf_str.
// Important note: For a pf_strv, the returned C-string may not necessarily
// be as long as the slice; it will be as long as the backing char * or pf_str
// instead.
#define PF_CSTRING(s) (s)._s

// Convert a pf_str to a pf_strv.
static inline pf_strv pf_STRTOVIEW(pf_str s)
{ return (pf_strv) { ._s = s._s, .len = s.len }; }

// Create a new pf_strv of size `len` from a C-string `buf`,
// where `buf` is a buffer managed by the caller that MUST to have a size of at
// least `len + 1` characters AND have `buf[len]` equal to '\0'.
pf_strv pf_strv_new_from_cstring(const char *buf, int len);

// Create a pf_strv from a string literal.
#define PF_STRLIT(lit) pf_strv_new_from_cstring((lit), sizeof((lit)) - 1)

// Create a new pf_str of size `len`.
// Allocates on the heap! Call pf_free to free the result.
pf_str pf_str_new_alloc(size_t len);

// Create a new formatted pf_str.
// Allocates on the heap! Call pf_str_free to free the result.
pf_str pf_str_new_alloc_formatted(const char *fmt, ...);

// Free a heap-allocated pf_str.
void pf_str_free(pf_str s);

void _pf_str_grow_with_strv(pf_str *a, pf_strv b);
void _pf_str_grow_with_str(pf_str *a, pf_str b);

// Grow a pf_str by concatenating another pf_str or pf_strv to it, in-place.
// Allocates on the heap! Reallocates `a` to fit `b`.
#define pf_str_grow(a, b)              \
    _Generic((b),                      \
        pf_str: _pf_str_grow_with_str, \
        pf_strv: _pf_str_grow_with_strv)((a), (b))

pf_str _pf_strv_concat(pf_strv a, pf_strv b);
pf_str _pf_str_concat(pf_str a, pf_str b);

// Concatenate two pf_strs or to pf_strvs with each other to produce a new
// pf_str.
// Allocates on the heap! Call pf_free to free the result.
#define pf_str_concat(a, b) \
    _Generic((a), pf_str: _pf_str_concat, pf_strv: _pf_strv_concat)((a), (b))

bool _pf_strv_equal(pf_strv a, pf_strv b);
bool _pf_str_equal(pf_str a, pf_str b);

// Determines whether two pf_strs or pf_strvs are equal in content.
#define pf_str_equal(a, b) \
    _Generic((a), pf_str: _pf_str_equal, pf_strv: _pf_strv_equal)((a), (b))

pf_strv _pf_strv_slice(pf_strv sv, size_t start, size_t end);
pf_strv _pf_str_slice(pf_str s, size_t start, size_t end);

// Given the pf_str or pf_strv `s`, return a pf_strv sliced
// from index `start` to `end`, exclusive at `end`.
// The slice indices are byte indices.
// If `start` or `end` is too small or too large in relation to the length
// of `s`, or if `start` is larger than `end`, an error message
// is printed and the program exits.
#define pf_str_slice(s, start, end)                                \
    _Generic((s), pf_str: _pf_str_slice, pf_strv: _pf_strv_slice)( \
        (s), (start), (end))

pf_strv _pf_strv_trim_left(pf_strv sv, const char *cutset);
pf_strv _pf_str_trim_left(pf_str s, const char *cutset);

// Create a pf_strv from the given `s` with any bytes that match the ones
// in `cutset` removed from the left end.
// `s` can be a pf_str or pf_strv.
#define pf_str_trim_left(s, cutset)                                        \
    _Generic((s), pf_str: _pf_str_trim_left, pf_strv: _pf_strv_trim_left)( \
        (s), (cutset))

pf_strv _pf_strv_trim_right(pf_strv sv, const char *cutset);
pf_strv _pf_str_trim_right(pf_str s, const char *cutset);

// Create a pf_strv from the given `s` with any bytes that match the ones
// in `cutset` removed from the right end.
// `s` can be a pf_str or pf_strv.
#define pf_str_trim_right(s, cutset)                                         \
    _Generic((s), pf_str: _pf_str_trim_right, pf_strv: _pf_strv_trim_right)( \
        (s), (cutset))

pf_strv _pf_strv_trim(pf_strv sv, const char *cutset);
pf_strv _pf_str_trim(pf_str s, const char *cutset);

// Create a pf_strv from the given `s` with any bytes that match the ones
// in `cutset` removed from the left and right ends.
// `s` can be a pf_str or pf_strv.
#define pf_str_trim(s, cutset) \
    _Generic((s), pf_str: _pf_str_trim, pf_strv: _pf_strv_trim)((s), (cutset))

pf_rune _pf_strv_index(pf_strv sv, size_t idx);
pf_rune _pf_str_index(pf_str s, size_t idx);

// Index the codepoint `idx` in `s` and return it as a pf_rune.
// `s` may be a pf_str or pf_strv.
// This is an O(n) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pf_str_index(s, idx) \
    _Generic((s), pf_str: _pf_str_index, pf_strv: _pf_strv_index)((s), (idx))

char _pf_strv_index_byte(pf_strv sv, size_t idx);
char _pf_str_index_byte(pf_str s, size_t idx);

// Index the byte `idx` in `s` and return it as a char.
// `s` may be a pf_str or pf_strv.
// This is an O(1) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pf_str_index_byte(s, idx)                                            \
    _Generic((s), pf_str: _pf_str_index_byte, pf_strv: _pf_strv_index_byte)( \
        (s), (idx))

void _pf_strv_print(pf_strv sv);
void _pf_str_print(pf_str s);

// Prints a pf_str or pf_strv.
// You could print using standard C functions like printf; but pf_print
// handles string slices as well as runes.
#define pf_str_print(s) \
    _Generic((s), pf_str: _pf_str_print, pf_strv: _pf_strv_print)((s))

typedef enum {
    _pf_str_iterator_iterating,
    _pf_str_iterator_done,
} _pf_str_iterator_state;  // State of the iterator.

// An iterator over codepoints in a pf_strv.
typedef struct {
    pf_strv _strv;                  // String view being iterated over.
    _pf_str_iterator_state _state;  // State of the iterator.
} pf_str_iterator;

pf_str_iterator _pf_strv_iterator_new(pf_strv sv);
pf_str_iterator _pf_str_iterator_new(pf_str sv);

// Create an iterator over codepoints of a pf_str or pf_strv.
#define pf_str_iterator_new(s)        \
    _Generic((s),                     \
        pf_str: _pf_str_iterator_new, \
        pf_strv: _pf_strv_iterator_new)((s))

// Decode and get the next codepoint from `it` as a pf_rune.
// If `len` is not NULL, the byte size of the codepoint is stored in it.
// If the iterator is done, it returns the 0 (the null character) and stores
// 0 in `len` (if it is not NULL). It is recommended to use
// pf_iterator_is_done to check if the iterator is done, though.
pf_rune pf_str_iterator_next(pf_str_iterator *it, size_t *len);

// Checks if the iterator `it` is exhausted.
bool pf_str_iterator_is_done(pf_str_iterator it);

#endif
