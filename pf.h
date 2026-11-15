/*
    pf.h: single header stdlib for C

USAGE
    Define this in your source:

    #define PF_IMPLEMENTATION
    #define "pf.h"

SECTIONS
    PANICS AND ASSERTIONS
        A small set of functions (or rather macros) for panics (panic, panicf)
        and assertions (assert, assertf) and aborting the program (fatal).

    STRINGS AND STRING VIEWS
        A string type (str) for strings with length information as well as a
        string view type (strv) to easily slice the string, along
        with various functions for working with them.

    CLI ARGUMENT PARSING
        Some simple machinery for parsing command line arguments.

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
#define panic pf_panic
#define panicf pf_panicf
#define fatal pf_fatal
#undef assert
#define assert pf_assert
#define assertf pf_assertf
#define str pf_str
#define strv pf_strv
#define rune pf_rune
#define rune_decode pf_rune_decode
#define rune_encode pf_rune_encode
#define RUNETOASCII PF_RUNETOASCII
#define STRTOVIEW PF_STRTOVIEW
#define strv_new_from_cstring pf_strv_new_from_cstring
#define STRLIT PF_STRLIT
#define str_new_alloc pf_str_new_alloc
#define str_new_alloc_formatted pf_str_new_alloc_formatted
#define str_free pf_str_free
#define str_to_cstring pf_str_to_cstring
#define strv_to_cstring_unsafe pf_strv_to_cstring_unsafe
#define strv_to_cstring_alloc pf_strv_to_cstring_alloc
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
#define ARG_MAX_FLAGS PF_ARG_MAX_FLAGS
#define ARG_MAX_SUBPARSERS PF_ARG_MAX_SUBPARSERS
#define argparser_arg_int pf_argparser_arg_int
#define argparser_arg_str pf_argparser_arg_str
#define argparser_arg_boolean pf_argparser_arg_boolean
#define argparser_arg_none pf_argparser_arg_none
#define argparser_arg_kind pf_argparser_arg_kind
#define argparser_arg pf_argparser_arg
#define argparser_flag pf_argparser_flag
#define argparser pf_argparser
#define argparser_make pf_argparser_make
#define argparser_destroy pf_argparser_destroy
#define argparser_flag_add pf_argparser_flag_add
#define argparser_print_usage pf_argparser_print_usage
#define argparse pf_argparse
#define LIST_INITIAL_CAP PF_LIST_INITIAL_CAP
#define list pf_list
#define list_init pf_list_init
#define list_len pf_list_len
#define list_cap pf_list_cap
#define list_append pf_list_append
#define list_get pf_list_get
#define list_pop pf_list_pop
#define list_free pf_list_free
#endif  // PF_NO_SHORT_NAMES

/// ----------------
/// Panics and assertions
/// ----------------

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

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
                                       int line, bool do_abort, const char *fmt,
                                       ...)
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
        abort();
    } else {
        va_list list;
        va_start(list, fmt);
        fprintf(stderr, "%s: ", prefix);
        vfprintf(stderr, fmt, list);
        va_end(list);
        putc('\n', stderr);
        fprintf(stderr, "    at line %d in file %s\n", line, file);
        exit(1);
    }
}

// End the program with an error message.
#define pf_panic(message) \
    panic_impl("panic", __FILE__, __LINE__, false, (message))

// End the program with a formatted error message.
#define pf_panicf(fmt, ...) \
    panic_impl("panic", __FILE__, __LINE__, false, (fmt), __VA_ARGS__)

// Abort the program with an error message.
#define pf_fatal(message) \
    panic_impl("fatal", __FILE__, __LINE__, true, (message))

// Assert the truth of a condition, panicking with the given error message if it
// is false.
#define pf_assert(expr, message)                                           \
    do {                                                                   \
        if (!(expr))                                                       \
            panic_impl("assertion `" #expr "` failed", __FILE__, __LINE__, \
                       false, (message));                                  \
    } while (0)

// Same as pf_assert but prints a formatted error message on failure.
#define pf_assertf(expr, fmt, ...)                                         \
    do {                                                                   \
        if (!(expr))                                                       \
            panic_impl("assertion `" #expr "` failed", __FILE__, __LINE__, \
                       false, (fmt), __VA_ARGS__);                         \
    } while (0)

// End the program with a formatted error message, indicating that an
// implementation is not yet done.
#define pf_todo(message) \
    panic_impl("todo", __FILE__, __LINE__, false, (message))

/// ----------------
/// Strings and string views
/// ----------------

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
{ return (r < 128) ? (int) r : -1; }

// Convert a pf_str to a pf_strv.
static inline pf_strv PF_STRTOVIEW(pf_str s)
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

// Get the underlying C-string for the given pf_str.
const char *pf_str_to_cstring(pf_str s);

// Return the underlying C-string for the given pf_strv.
// Warning! This is unsafe because the underlying string may not contain a
// null terminator!
const char *pf_strv_to_cstring_unsafe(pf_strv s);

// Create a properly null-terminated C-string for the given pf_strv.
// Allocates on the heap! Call your relevant allocator's free function to free
// the result.
const char *pf_strv_to_cstring_alloc(pf_strv s);

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

/// ----------------
/// CLI argument parsing
/// ----------------

#define PF_ARG_MAX_FLAGS 16
#define PF_ARG_MAX_SUBPARSERS 16

typedef enum {
    pf_argparser_arg_int,      // The `int` type.
    pf_argparser_arg_str,      // A `const char *`.
    pf_argparser_arg_boolean,  // A `_Bool`/`bool`, or any integer type.
    pf_argparser_arg_none,     // No argument.
} pf_argparser_arg_kind;

// An argument taken by a flag or subcommand.
typedef struct {
    // Value of the argument, it points to a specified storage location by
    // the user.
    // Avoid accessing this data directly.
    void *_value;

    // Description of what the argument is. Can be left NULL to not provide
    // info about it.
    const char *description;

    // The type of _arg.
    // A point about 'boolean': it is similar to 'none' in how it is parsed in
    // the sense that it doesn't actually read any command line arg for its
    // value. The presence of the argument is enough for the value to be true.
    pf_argparser_arg_kind kind;
} pf_argparser_arg;

typedef struct {
    // Name of the flag.
    const char *name;

    // Short form of the flag. Can be NULL, if the flag does not have one.
    const char *short_name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Argument taken by the flag.
    pf_argparser_arg arg;
} pf_argparser_flag;

// Parser for a command. Has a set of its own subparsers.
// and flags.
typedef struct pf_argparser {
    // Name of the command.
    const char *name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Subparsers to parse any subcommands.
    struct pf_argparser *subparsers[PF_ARG_MAX_SUBPARSERS];
    int subparser_count;

    // Flags.
    pf_argparser_flag *flags[PF_ARG_MAX_FLAGS];
    int flag_count;

    // Argument taken by the subcommand.
    pf_argparser_arg arg;
} pf_argparser;

// Create an argument parser as the subparser of `parent`, with a subcommand
// name `name`, description `description`, argument kind `kind`, and argument
// description `arg_description`.
// If the parser is the root parser (has no parents), `parent` must be set
// to `NULL`.
// The argument the command takes is provided via `kind`, alongside the storage
// region for that value provided in `arg_ptr`.
// If the command takes no argument (`kind` == 'pf_argparser_arg_none'),
// `arg_ptr` can be set to `NULL`. If the provided parser spec has any issues
// (e.g. attempt to add a duplicate subcommand, attempt to add a subparser for a
// parser that takes an argument, addition of too many subparsers), the function
// prints the error to stderr and exits the program with status 1. `arg_ptr`
// will be one of these respective values in the case the subcommand is never
// provided (and thus no argument ever parsed):
//      - 'i': 0xdeadbeef
//      - 's': ""
//      - 'b': false
// `arg_description` can be set to NULL if it is not desired or applicable.
pf_argparser *pf_argparser_make(pf_argparser *parent, const char *name,
                                const char *description,
                                pf_argparser_arg_kind kind, void *arg_ptr,
                                const char *arg_description);

// Deallocate an argument parser.
// Also deallocates all of its subparsers and flags; hence you should avoid
// running it for every single subparser; just destroy the root.
void pf_argparser_destroy(pf_argparser *parent);

// Add a flag to `parser`, with a `name` and optional `short_name` and
// `description`. The latter two can be left NULL if undesired. The type of the
// argument the flag takes is provided via `kind`, alongside the storage region
// for that value provided in `arg_ptr`.
// If the flag takes no argument (`kind` == 'pf_argparser_arg_none'), `arg_ptr`
// can be set to `NULL`. `arg_description` is the description for the argument
// shown in usage messages, it can be set to NULL if not desired or applicable.
// If the provided flag spec has any issues (e.g. invalid flag name, attempt to
// add a duplicate flag, addition of too many flags, invalid argument kind), the
// function prints the error to stderr and exits the program with status 1.
// `arg_ptr` will be one of these respective values in the case the flag is
// never provided (and thus no argument ever parsed):
//      - 'i': 0xdeadbeef
//      - 's': ""
//      - 'b': false
// `arg_description` can be set to NULL if it is not desired or applicable.
void pf_argparser_flag_add(pf_argparser *parser, const char *name,
                           const char *short_name, const char *description,
                           pf_argparser_arg_kind kind, void *arg_ptr,
                           const char *arg_description);

// Print the usage message for `parser`.
void pf_argparser_print_usage(pf_argparser *parser);

// Parse arguments based on the spec in `parser`.
// Returns the number of arguments parsed on success (a non-negative integer),
// otherwise returns -1.
// If it encounters a problem with the setup of the arguments (for example an
// invalid argument kind, or invalid `argv`), it prints the issue to stderr and
// exits the program with status 1.
int pf_argparse(pf_argparser *parser, int argc, const char *argv[]);

/// ----------------
/// Lists
/// ----------------

#include <stdalign.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

// Starting size of a pf_list after the first append.
#define PF_LIST_INITIAL_CAP 8

#define _PF_LIST_ALIGNMENT alignof(max_align_t)
#define _PF_LIST_HEADER_SIZE                                                \
    (((2 * sizeof(size_t) + _PF_LIST_ALIGNMENT - 1) / _PF_LIST_ALIGNMENT) * \
     _PF_LIST_ALIGNMENT)
#define _PF_LIST_HEADER(l) \
    ((size_t *) ((unsigned char *) (l) - _PF_LIST_HEADER_SIZE))

// Type of a growable dynamic array for a certain type T.
// T can be any type, except for an over-aligned type; they are not supported.
// Pointers to elements within a list are NOT guaranteed to stay valid.
#define pf_list(T) T *

// Create a new empty pf_list.
#define pf_list_init(T) (T *) NULL

// Get the number of items in the given pf_list.
#define pf_list_len(l) ((l) == NULL ? 0 : (_PF_LIST_HEADER(l))[0])

// Get the number of items the given pf_list can currently hold.
#define pf_list_cap(l) ((l) == NULL ? 0 : (_PF_LIST_HEADER(l))[1])

// Add the provided item to the end of the list `l`.
// `l` MUST be a variable that refers to a pf_list, unexpected bad things will
// happen if it is not!
// Allocates on the heap if the provided list is empty or if it doesn't have
// enough capacity for the new item.
#define pf_list_append(l, item)                                                \
    do {                                                                       \
        if ((l) == NULL) {                                                     \
            void *buf = malloc(_PF_LIST_HEADER_SIZE +                          \
                               (PF_LIST_INITIAL_CAP * sizeof(*(l))));          \
            if (!buf) {                                                        \
                pf_fatal("out of memory!");                                    \
            }                                                                  \
            size_t *header = buf;                                              \
            header[0] = 0;                                                     \
            header[1] = PF_LIST_INITIAL_CAP;                                   \
            (l) = (void *) ((unsigned char *) buf + _PF_LIST_HEADER_SIZE);     \
        }                                                                      \
        size_t *header = _PF_LIST_HEADER(l);                                   \
        size_t len = header[0];                                                \
        size_t cap = header[1];                                                \
        if (len >= cap) {                                                      \
            if (cap > (SIZE_MAX / 2)) {                                        \
                pf_panicf("attempt to grow a pf_list over capacity limit of "  \
                          "%zu items",                                         \
                          SIZE_MAX);                                           \
            }                                                                  \
            size_t newcap = cap * 2;                                           \
            if (newcap > (SIZE_MAX / sizeof(*(l)))) {                          \
                pf_panicf("attempt to grow a pf_list over byte size limit of " \
                          "%zu bytes",                                         \
                          SIZE_MAX);                                           \
            }                                                                  \
            void *buf = realloc(header, _PF_LIST_HEADER_SIZE +                 \
                                            (newcap * sizeof(*(l))));          \
            if (!buf) {                                                        \
                pf_fatal("out of memory!");                                    \
            }                                                                  \
            header = buf;                                                      \
            header[1] = newcap;                                                \
            (l) = (void *) ((unsigned char *) buf + _PF_LIST_HEADER_SIZE);     \
        }                                                                      \
        l[len] = (item);                                                       \
        header[0] = len + 1;                                                   \
    } while (0)

// Get an element from the list `l` at index `i`.
// It is essentially the same as l[i], however it is bounds checked and will
// therefore panic if the given index is outside the bounds of the list.
// `l` MUST be a variable that refers to a pf_list, unexpected bad things will
// happen if it is not!
#define pf_list_get(l, i)                                                  \
    (i >= 0 && i < pf_list_len(l)                                          \
         ? (l)[i]                                                          \
         : (pf_panicf(                                                     \
                "attempt to index a pf_list of length %zu with index %zu", \
                pf_list_len(l), i),                                        \
            (l)[0]))

// Remove the last element from the list and return it.
// `l` MUST be a variable that refers to a pf_list, unexpected bad things will
// happen if it is not!
// If the given list is of size 0, panics.
#define pf_list_pop(l)                  \
    (pf_list_len(l) > 0                 \
         ? (l)[--_PF_LIST_HEADER(l)[0]] \
         : (pf_panic("attempt to pop a pf_list that is empty"), (l)[0]))

// Free the heap memory of the provided pf_list.
// `l` MUST be a variable that refers to a pf_list, unexpected bad things will
// happen if it is not!
// It is safe to pass an empty pf_list here.
#define pf_list_free(l)                                         \
    do {                                                        \
        if ((l) != NULL) {                                      \
            free((unsigned char *) (l) - _PF_LIST_HEADER_SIZE); \
            (l) = NULL;                                         \
        }                                                       \
    } while (0)

#endif  // PF_H

/// ----------------
/// Implementation
/// ----------------

#ifdef PF_IMPLEMENTATION
#ifndef _PF_IMPLEMENTED
#define _PF_IMPLEMENTED

/// ----------------
/// Strings and string views
/// ----------------

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

pf_rune pf_rune_decode(char *s, size_t *len)
{
    pf_rune cp;
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

int pf_rune_encode(pf_rune cp, char out[4])
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

pf_strv pf_strv_new_from_cstring(const char *buf, int len)
{
    return (pf_strv) {
        ._s = buf,
        .len = len,
    };
}

pf_str pf_str_new_alloc(size_t len)
{
    char *buf = malloc(len + 1);  // for null terminator
    if (!buf) {
        pf_fatal("out of memory!");
    }
    buf[len] = '\0';
    return (pf_str) {
        ._s = buf,
        .len = len,
    };
}

pf_str pf_str_new_alloc_formatted(const char *fmt, ...)
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

    return (pf_str) { ._s = buf, .len = len };
}

void pf_str_free(pf_str s)
{
    if (s._s == NULL)
        return;
    free(s._s);
    s._s = NULL;
}

const char *pf_str_to_cstring(pf_str s)
{ return s._s; }

const char *pf_strv_to_cstring_unsafe(pf_strv s)
{ return s._s; }

const char *pf_strv_to_cstring_alloc(pf_strv s)
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
void _pf_str_grow_with_strv(pf_str *a, pf_strv b)
{
    if (a->len > SIZE_MAX - b.len - 1) {
        pf_panicf("attempt to grow a pf_str over size limit of %zu bytes",
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

void _pf_str_grow_with_str(pf_str *a, pf_str b)
{ _pf_str_grow_with_strv(a, PF_STRTOVIEW(b)); }

pf_str _pf_strv_concat(pf_strv a, pf_strv b)
{
    if (a.len > SIZE_MAX - b.len - 1) {
        pf_panicf("attempt to create a concatenated pf_str over "
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
    return (pf_str) { ._s = buf, .len = len };
}

pf_str _pf_str_concat(pf_str a, pf_str b)
{ return _pf_strv_concat(PF_STRTOVIEW(a), PF_STRTOVIEW(b)); }

bool _pf_strv_equal(pf_strv a, pf_strv b)
{
    if (a.len != b.len)
        return false;

    for (size_t i = 0; i < a.len; i++)
        if (a._s[i] != b._s[i])
            return false;

    return true;
}

bool _pf_str_equal(pf_str a, pf_str b)
{ return _pf_strv_equal(PF_STRTOVIEW(a), PF_STRTOVIEW(b)); }

pf_strv _pf_strv_slice(pf_strv sv, size_t start, size_t end)
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

    return (pf_strv) {
        ._s = sv._s + start,
        .len = end - start,
    };
}

pf_strv _pf_str_slice(pf_str s, size_t start, size_t end)
{ return _pf_strv_slice(PF_STRTOVIEW(s), start, end); }

pf_rune _pf_strv_index(pf_strv sv, size_t idx)
{
    const char *p = pf_strv_to_cstring_unsafe(sv);
    size_t cps = 0;
    size_t bytes = 0;
    while (bytes < sv.len) {
        size_t len;
        pf_rune cp = pf_rune_decode((char *) p, &len);
        if (cps == idx)
            return cp;

        p += len;
        bytes += len;
        cps += 1;
    }

    pf_panicf("invalid index %zu for s with %d codepoints", idx, cps);
}

pf_rune _pf_str_index(pf_str s, size_t idx)
{ return _pf_strv_index(PF_STRTOVIEW(s), idx); }

char _pf_strv_index_byte(pf_strv sv, size_t idx)
{
    if (idx >= sv.len) {
        pf_panicf("invalid byte index %zu for s of length %zu", idx, sv.len);
    }

    return sv._s[idx];
}

char _pf_str_index_byte(pf_str s, size_t idx)
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

pf_strv _pf_strv_trim_left(pf_strv sv, const char *cutset)
{
    if (sv.len == 0 || (strcmp(cutset, "") == 0))
        return sv;

    pf_strv res = sv;
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

pf_strv _pf_str_trim_left(pf_str s, const char *cutset)
{ return _pf_strv_trim_left(PF_STRTOVIEW(s), cutset); }

pf_strv _pf_strv_trim_right(pf_strv sv, const char *cutset)
{
    if (sv.len == 0 || (strcmp(cutset, "") == 0))
        return sv;

    pf_strv res = sv;
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

pf_strv _pf_str_trim_right(pf_str s, const char *cutset)
{ return _pf_strv_trim_right(PF_STRTOVIEW(s), cutset); }

pf_strv _pf_strv_trim(pf_strv sv, const char *cutset)
{
    pf_strv left = _pf_strv_trim_left(sv, cutset);
    return _pf_strv_trim_right(left, cutset);
}

pf_strv _pf_str_trim(pf_str s, const char *cutset)
{ return _pf_strv_trim(PF_STRTOVIEW(s), cutset); }

void _pf_strv_print(pf_strv sv)
{
    for (size_t i = 0; i < sv.len; i++)
        putchar(sv._s[i]);
}

void _pf_str_print(pf_str s)
{ _pf_strv_print(PF_STRTOVIEW(s)); }

pf_str_iterator _pf_strv_iterator_new(pf_strv sv)
{
    _pf_str_iterator_state state = _pf_str_iterator_iterating;
    if (sv.len == 0)
        state = _pf_str_iterator_done;
    return (pf_str_iterator) { ._strv = sv, ._state = state };
}

pf_str_iterator _pf_str_iterator_new(pf_str s)
{ return _pf_strv_iterator_new(PF_STRTOVIEW(s)); }

pf_rune pf_str_iterator_next(pf_str_iterator *it, size_t *len)
{
    pf_rune cp;
    size_t cplen;

    if (it->_state == _pf_str_iterator_iterating) {
        pf_strv sv = it->_strv;
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

    return (pf_rune) cp;
}

bool pf_str_iterator_is_done(pf_str_iterator it)
{ return it._state == _pf_str_iterator_done; }

/// ----------------
/// CLI argument parsing
/// ----------------

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline const char *pf_argument_kind_str(pf_argparser_arg_kind k)
{
    switch (k) {
    case pf_argparser_arg_none:
        return "none";
    case pf_argparser_arg_int:
        return "int";
    case pf_argparser_arg_boolean:
        return "boolean";
    case pf_argparser_arg_str:
        return "string";
    }

    pf_panicf("invalid argument kind str %d", k);
}

pf_argparser *pf_argparser_make(pf_argparser *parent, const char *name,
                                const char *description,
                                pf_argparser_arg_kind kind, void *arg_ptr,
                                const char *arg_description)
{
    pf_assert(name != NULL, "parser_make: parser name cannot be NULL");

    if (parent != NULL) {
        // Ensure we're not adding too many subparsers
        pf_assertf(parent->subparser_count < PF_ARG_MAX_SUBPARSERS,
                   "parser_make: failed to add subparser %s to parent %s, "
                   "parent has too many subparsers; only upto %d supported",
                   name, parent->name, PF_ARG_MAX_SUBPARSERS);

        // A parser cannot have a subparser if it takes an argument
        pf_assertf(
            parent->arg.kind == pf_argparser_arg_none,
            "parser_make: cannot add subparser %s to parser %s because the "
            "latter takes a %s argument",
            name, parent->name, pf_argument_kind_str(parent->arg.kind));

        // Ensure no duplicates
        for (int i = 0; i < parent->subparser_count; i++) {
            pf_assertf(
                strcmp(name, parent->subparsers[i]->name) != 0,
                "parser_make: cannot add subcommand %s twice to parent %s",
                name, parent->name);
        }
    }

    pf_argparser *p = malloc(sizeof(pf_argparser));
    if (!p) {
        pf_fatal(strerror(errno));
    }

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case pf_argparser_arg_int:
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case pf_argparser_arg_str:
        *((const char **) arg_ptr) = "";
        break;
    case pf_argparser_arg_boolean:
        *((bool *) arg_ptr) = false;
        break;
    case pf_argparser_arg_none:
        break;  // nothing to do
    default:
        pf_panicf("invalid argument kind %d", kind);
    }

    *p = (pf_argparser) {
        .name = name,
        .description = description,
        .subparser_count = 0,
        .flag_count = 0,
        .arg.kind = kind,
        .arg.description = arg_description,
        .arg._value = arg_ptr,
    };

    memset(p->subparsers, 0, sizeof p->subparsers);
    memset(p->flags, 0, sizeof p->flags);

    // Add --help flag
    pf_argparser_flag_add(p, "--help", "-h", "Display help message",
                          pf_argparser_arg_none, NULL, NULL);

    if (parent != NULL) {
        parent->subparsers[parent->subparser_count] = p;
        parent->subparser_count++;
    }

    return p;
}

void pf_argparser_destroy(pf_argparser *parent)
{
    for (int i = 0; i < parent->subparser_count; i++) {
        pf_argparser_destroy(parent->subparsers[i]);
    }

    for (int i = 0; i < parent->flag_count; i++) {
        free(parent->flags[i]);
    }

    free(parent);
}

void pf_argparser_flag_add(pf_argparser *parser, const char *name,
                           const char *short_name, const char *description,
                           pf_argparser_arg_kind kind, void *arg_ptr,
                           const char *arg_description)
{
    pf_assert(name != NULL, "flag_add: long name of flag cannot be NULL");

    // Ensure not too many flags
    pf_assertf(parser->flag_count < PF_ARG_MAX_FLAGS,
               "flag_add: failed to add flag %s to parser %s, "
               "parser has too many flags; only upto %d supported",
               name, parser->name, PF_ARG_MAX_FLAGS);

    // Flag must begin with --
    pf_assertf(strncmp(name, "--", 2) == 0,
               "flag_add: invalid flag name %s, it must begin with `--`", name);

    if (short_name != NULL) {
        // Short flag must begin with -
        pf_assertf(
            strncmp(short_name, "-", 1) == 0,
            "flag_add: invalid short flag name %s, it must begin with `-`",
            name);

        // Short flag must be exactly two chars long
        pf_assertf(strlen(short_name) == 2,
                   "flag_add: invalid short flag name %s, it must be "
                   "exactly two characters long",
                   short_name);
    }

    // Ensure no duplicates, and that the --help or -h flags are not duplicated.
    // The latter is the same as the duplicate problem, but special cased to
    // provide a clearer error message.
    for (int i = 0; i < parser->flag_count; i++) {
        pf_assertf(strcmp(name, parser->flags[i]->name) != 0,
                   "flag_add: cannot add flag %s twice", name);

        if (short_name != NULL && parser->flags[i]->short_name != NULL) {
            pf_assertf(strcmp(short_name, parser->flags[i]->short_name) != 0,
                       "flag_add: cannot add flag with short name %s twice",
                       short_name);
        }
    }

    pf_argparser_flag *f = malloc(sizeof(pf_argparser_flag));
    if (!f) {
        pf_fatal(strerror(errno));
    }

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case pf_argparser_arg_int:
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case pf_argparser_arg_str:
        *((const char **) arg_ptr) = "";
        break;
    case pf_argparser_arg_boolean:
        *((bool *) arg_ptr) = false;
        break;
    case pf_argparser_arg_none:
        break;  // nothing to do
    default:
        pf_panicf("invalid argument kind %d", kind);
    }

    *f = (pf_argparser_flag) {
        .name = name,
        .description = description,
        .short_name = short_name,
        .arg.kind = kind,
        .arg.description = arg_description,
        .arg._value = arg_ptr,
    };

    parser->flags[parser->flag_count] = f;
    parser->flag_count++;
}

// Print a parser usage message.
// This is the output of the program when the --help or -h flag is invoked, and
// is also shown on argument parsing errors.
void pf_argparser_print_usage(pf_argparser *parser)
{
    printf("%s", parser->name);
    if (parser->description != NULL) {
        printf(": %s", parser->description);
    }
    printf("\n\n");

    printf("Usage:\n");
    printf("    ");
    if (parser->arg.kind == pf_argparser_arg_none) {
        printf("%s", parser->name);

        if (parser->subparser_count > 0) {
            printf(" <subcommand>");
        }

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }
    } else {
        printf("%s <%s>", parser->name, pf_argument_kind_str(parser->arg.kind));

        // No subparser_count check here, as a parser that takes an argument
        // is NOT allowed to have any subcommands

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }

        if (parser->arg.description != NULL) {
            printf("\n\n");
            printf("Argument of type %s: %s",
                   pf_argument_kind_str(parser->arg.kind),
                   parser->arg.description);
        }
    }
    printf("\n\n");

    printf("Subcommands:\n");
    if (parser->subparser_count == 0) {
        printf("    None\n");
    } else {
        for (int i = 0; i < parser->subparser_count; i++) {
            pf_argparser *subparser = parser->subparsers[i];

            printf("    ");
            printf("%s", subparser->name);
            if (subparser->description != NULL) {
                printf("    %s", subparser->description);
            }
            printf("\n");
        }
    }
    printf("\n");

    printf("Flags:\n");
    if (parser->flag_count == 0) {
        printf("    None\n");
    } else {
        for (int i = 0; i < parser->flag_count; i++) {
            pf_argparser_flag *flag = parser->flags[i];

            printf("    ");
            printf("%s", flag->name);
            if (flag->short_name != NULL) {
                printf(", %s", flag->short_name);
            }

            if (flag->description != NULL) {
                printf("    %s", flag->description);
            }
            printf("\n");
        }
    }
}

// Attempt to parse an argument. It is considered an error if no valid argument
// is found.
// Returns the number of arguments parsed on success, and -1 on error.
static int pf_parse_argument(pf_argparser *parser, int argc, const char *argv[])
{
    if (argc == 0 && parser->arg.kind != pf_argparser_arg_none &&
        parser->arg.kind != pf_argparser_arg_boolean) {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pf_argument_kind_str(parser->arg.kind), parser->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (parser->arg.kind) {
    case pf_argparser_arg_none:
        break;
    case pf_argparser_arg_boolean:
        *((bool *) parser->arg._value) = true;
        break;
    case pf_argparser_arg_int: {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            pf_panicf("parse: integer overflow on %ld", v);

        //  No digits found in string
        if (endptr == arg) {
            fprintf(stderr, "parse: %s is not a valid integer", arg);
            return -1;
        }

        // Entire string not consumed (string is not entirely numeric)
        if (*endptr != '\0') {
            fprintf(stderr, "parse: %s is not a valid integer", arg);
            return -1;
        }

        *((int *) parser->arg._value) = (int) v;

        argv++;
        argc--;
        break;
    }
    case pf_argparser_arg_str: {
        const char *arg = argv[0];
        *((const char **) parser->arg._value) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        pf_panicf("parse: invalid argument kind %d", parser->arg.kind);
    }

    return initial_argc - argc;
}

// Attempt to parse an argument of a flag. It is considered an error if no valid
// argument is found.
// Returns the number of arguments parsed on success, and -1 on error.
static int pf_parse_argument_flag(pf_argparser_flag *flag, int argc,
                                  const char *argv[])
{
    if (argc == 0 && flag->arg.kind != pf_argparser_arg_none &&
        flag->arg.kind != pf_argparser_arg_boolean) {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pf_argument_kind_str(flag->arg.kind), flag->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (flag->arg.kind) {
    case pf_argparser_arg_none:
        break;
    case pf_argparser_arg_boolean:
        *((bool *) flag->arg._value) = true;
        break;
    case pf_argparser_arg_int: {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            pf_panicf("parse: integer overflow on %ld", v);

        //  No digits found in string
        if (endptr == arg) {
            fprintf(stderr, "parse: %s is not a valid integer", arg);
            return -1;
        }

        // Entire string not consumed (string is not entirely numeric)
        if (*endptr != '\0') {
            fprintf(stderr, "parse: %s is not a valid integer", arg);
            return -1;
        }

        *((int *) flag->arg._value) = (int) v;

        argv++;
        argc--;
        break;
    }
    case pf_argparser_arg_str: {
        const char *arg = argv[0];
        *((const char **) flag->arg._value) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        pf_panicf("parse: invalid argument kind %d", flag->arg.kind);
    }

    return initial_argc - argc;
}

// Parse flags if found. If no args are remaining it doesn't do anything.
// Returns a whole number (0 or higher) on success, which is the number of flags
// parsed. Returns -1 on error.
static int pf_parse_flags(pf_argparser *parser, int argc, const char *argv[])
{
    if (argc == 0)
        return 0;

    int initial_argc = argc;

    // If we find a flag, start parsing them
    while (argc > 0 &&
           (strncmp(argv[0], "--", 2) == 0 || strncmp(argv[0], "-", 1) == 0)) {
        const char *flag_name = argv[0];
        pf_argparser_flag *received_flag = NULL;

        for (int i = 0; i < parser->flag_count; i++) {
            if (strcmp(flag_name, parser->flags[i]->name) == 0 ||
                (parser->flags[i]->short_name &&
                 strcmp(flag_name, parser->flags[i]->short_name) == 0)) {
                received_flag = parser->flags[i];
                break;
            }
        }

        if (received_flag == NULL) {
            fprintf(stderr, "parse: unknown flag %s\n", flag_name);
            pf_argparser_print_usage(parser);
            return -1;
        }

        // If we received the help flag just print usage and exit
        if (strcmp(received_flag->name, "--help") == 0 ||
            (received_flag->short_name != NULL &&
             strcmp(received_flag->short_name, "-h") == 0)) {
            pf_argparser_print_usage(parser);
            exit(0);
        }

        argv++;
        argc--;

        // Try to parse argument
        int skipped = pf_parse_argument_flag(received_flag, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    return initial_argc - argc;
}

int pf_argparse(pf_argparser *parser, int argc, const char *argv[])
{
    int initial_argc = argc;
    int skipped;

    pf_assert(argc >= 1,
              "parse: invalid arguments provided, need at least 1 arg\n");

    // Consume the first arg (program / subcommand name)
    argv++;
    argc--;

    // Try to parse flags
    skipped = pf_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // If the parser has any subparsers, parse them recursively
    if (argc > 0 && parser->subparser_count > 0) {
        const char *subcommand_name = argv[0];
        pf_argparser *received_subparser = NULL;
        for (int i = 0; i < parser->subparser_count; i++) {
            if (strcmp(subcommand_name, parser->subparsers[i]->name) == 0) {
                received_subparser = parser->subparsers[i];
                break;
            }
        }

        if (received_subparser == NULL) {
            fprintf(stderr, "parse: unknown subcommand %s\n", subcommand_name);
            goto die;
        }

        skipped = pf_argparse(received_subparser, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    // Try to parse flags... again
    skipped = pf_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // Try to parse argument
    skipped = pf_parse_argument(parser, argc, argv);
    if (skipped == -1)
        goto die;

    argv += skipped;
    argc -= skipped;

    // Parse any remaining flags...
    skipped = pf_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    return initial_argc - argc;

die:
    pf_argparser_print_usage(parser);
    return -1;
}

#endif  // _PF_IMPLEMENTED
#endif  // PF_IMPLEMENTATION
