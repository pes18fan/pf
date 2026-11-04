#ifndef pf_str_h
#define pf_str_h

#include <stdarg.h>
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

// Returns an ASCII `char` for the given pf_rune if it is in range. Otherwise
// it returns -1.
static inline char PF_RUNETOASCII(pf_rune r)
{ return r < 128 ? (char) (r & 127) : -1; }

// Get the cstring representation of a pf_str or pf_strv.
#define PF_CSTRING(str) (str)._s

// Convert a pf_str to a pf_strv.
static inline pf_strv PF_TOVIEW(pf_str str)
{ return (pf_strv) { ._s = str._s, .len = str.len }; }

// Create a new pf_strv of size `len` from a preallocated buffer `buf`,
// where `buf` is assumed to have a size of at least `len + 1` characters and
// have `buf[len]` equal to '\0'.
pf_strv _pf_strv_new_from_nullterminated_buf(const char *buf, int len);

// Create a new pf_strv for a string of size `len` from a preallocated buffer
// `buf`.
// `buf[len]` is set to a null character here, therefore it MUST be able to hold
// at least `len + 1` characters.
pf_strv pf_strv_new_from_buf(char *buf, int len);

// Create a pf_strv from a string literal.
#define PF_STRLIT(lit) \
    _pf_strv_new_from_nullterminated_buf((lit), sizeof((lit)) - 1)

// Create a new pf_str of size `len` by allocating on the heap.
// When you are done using it, remember to call pf_str_free!
pf_str pf_str_new_alloc(int len);

// Create a new formatted pf_str by allocating on the heap.
// When you are done using it, remember to call pf_str_free!
pf_str pf_str_new_alloc_formatted(const char *fmt, ...);

// Free a heap-allocated pf_str.
void pf_str_free(pf_str str);

bool _pf_strv_equal(pf_strv a, pf_strv b);
bool _pf_str_equal(pf_str a, pf_str b);

// Determines whether two pf_strs or pf_strvs are equal in content.
#define pf_str_equal(a, b) \
    _Generic((a), pf_str: _pf_str_equal, pf_strv: _pf_strv_equal)(a, b)

pf_strv _pf_strv_slice(pf_strv strv, int start, int end);
pf_strv _pf_str_slice(pf_str str, int start, int end);

// Given the pf_str or pf_strv `str`, return a pf_strv sliced
// from index `start` to `end`, exclusive at `end`.
// If `start` or `end` is too small or too large in relation to the length
// of `str`, or if `start` is larger than `end`, an error message
// is printed and the program exits.
#define pf_str_slice(str, start, end)                                 \
    _Generic((str), pf_str: _pf_strv_slice, pf_strv: _pf_strv_slice)( \
        (str), (start), (end))

pf_rune _pf_strv_index(pf_strv strv, int idx);
pf_rune _pf_str_index(pf_str str, int idx);

// Index the codepoint `idx` in `str` and return it as a pf_rune.
// `str` may be a pf_str or pf_strv.
// This is an O(n) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pf_str_index(str, idx)                                              \
    _Generic((str), pf_str: _pf_strv_index, pf_strv: _pf_strv_index)((str), \
                                                                     (idx))

char _pf_strv_index_byte(pf_strv strv, int idx);
char _pf_str_index_byte(pf_str str, int idx);

// Index the byte `idx` in `strv` and return it as a char.
// `str` may be a pf_str or pf_strv.
// This is an O(1) operation.
// If the index is too small or too large, an error message
// is printed and the program exits.
#define pf_str_index_byte(str, idx)  \
    _Generic((str),                  \
        pf_str: _pf_strv_index_byte, \
        pf_strv: _pf_strv_index_byte)((str), (idx))

void _pf_strv_print(pf_strv strv);
void _pf_str_print(pf_str str);

// Prints a pf_str or pf_strv.
// You could print using standard C functions like printf; but pf_str_print
// handles string slices as well as runes.
#define pf_str_print(str) \
    _Generic((str), pf_str: _pf_strv_print, pf_strv: _pf_strv_print)((str))

// Decodes and prints a pf_rune.
void pf_rune_print(pf_rune rune);

typedef enum {
    _pf_str_iterator_iterating,
    _pf_str_iterator_done,
} pf_str_iterator_state;  // State of the iterator.

// An iterator over codepoints in a pf_strv.
typedef struct {
    pf_strv _strv;                 // String view being iterated over.
    pf_str_iterator_state _state;  // State of the iterator.
} pf_str_iterator;

pf_str_iterator _pf_strv_iterator_new(pf_strv strv);
pf_str_iterator _pf_str_iterator_new(pf_str strv);

// Create an iterator over codepoints of a pf_str or pf_strv.
#define pf_str_iterator_new(str)      \
    _Generic((str),                   \
        pf_str: _pf_str_iterator_new, \
        pf_strv: _pf_strv_iterator_new)((str))

// Decode and get the next codepoint from `it` as a pf_rune.
// If `len` is not NULL, the byte size of the codepoint is stored in it.
// If the iterator is done, it returns the 0 (the null character) and stores
// 0 in `len` (if it is not NULL). It is recommended to use
// pf_str_iterator_is_done to check if the iterator is done, though.
pf_rune pf_str_iterator_next(pf_str_iterator *it, size_t *len);

// Checks if the iterator `it` is exhausted.
bool pf_str_iterator_is_done(pf_str_iterator it);

#endif
