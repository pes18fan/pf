#include "pf_str.h"

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Decode a single codepoint from a UTF-8 encoded string.
// Returns the codepoint, and stores its byte size in `len`.
// Function definition adapted from the one shown in Jimmy Lefevre's talk at
// Better Software Conference 2026.
static uint32_t decode_utf8(char *s, size_t *len)
{
    uint32_t cp;
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

// Encode a codepoint to UTF-8.
// Returns byte size of the encoded codepoint, and the encoded bytes in `out`.
// If `cp` is not a valid Unicode codepoint, -1 is returned.
static int encode_utf8(uint32_t cp, char out[4])
{
    size_t len;
    if (cp <= 0x7f) {
        out[0] = cp;
        len = 1;
    } else if (cp <= 0x7ff) {
        out[0] = 0xc0 | (cp >> 6);
        out[1] = 0x80 | (cp & 0x3f);
        len = 2;
    } else if (cp <= 0xd7ff) {
        out[0] = 0xe0 | (cp >> 12);
        out[1] = 0x80 | ((cp >> 6) & 0x3f);
        out[2] = 0x80 | (cp & 0x3f);
        len = 3;
    } else if (cp >= 0xe000 && cp <= 0x10ffff) {
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

pf_strv _pf_strv_new_from_nullterminated_buf(const char *buf, int len)
{
    return (pf_strv){
        ._s = buf,
        .len = len,
    };
}

pf_strv pf_strv_new_from_buf(char *buf, int len)
{
    buf[len] = '\0';
    return _pf_strv_new_from_nullterminated_buf(buf, len);
}

pf_str pf_str_new_alloc(int len)
{
    char *buf = malloc(len + 1);  // for null terminator
    if (!buf) {
        fprintf(stderr, "fatal: out of memory!\n");
        exit(1);
    }
    buf[len] = '\0';
    return (pf_str){
        ._s = buf,
        .len = len,
    };
}

void pf_str_free(pf_str str)
{ free(str._s); }

pf_str pf_str_new_alloc_formatted(const char *fmt, ...)
{
    va_list args, args2;
    va_start(args, fmt);
    va_copy(args2, args);
    int len = vsnprintf(NULL, 0, fmt, args);
    va_end(args);

    if (len < 0) {
        va_end(args2);
        fprintf(stderr, "fatal: failed to format string\n");
        exit(1);
    }

    char *buf = malloc(len + 1);
    if (!buf) {
        va_end(args2);
        fprintf(stderr, "fatal: out of memory!\n");
        exit(1);
    }

    vsnprintf(buf, len + 1, fmt, args2);
    va_end(args2);

    return (pf_str){ ._s = buf, .len = len };
}

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
{ return _pf_strv_equal(PF_TOVIEW(a), PF_TOVIEW(b)); }

pf_strv _pf_strv_slice(pf_strv strv, int start, int end)
{
    if (start > end) {
        fprintf(stderr,
                "fatal: starting slice index %d is larger than ending "
                "index %d\n",
                start, end);
        exit(1);
    }

    if (start < 0 || start > strv.len) {
        fprintf(stderr,
                "fatal: invalid starting slice index %d for str of length "
                "%zu\n",
                start, strv.len);
        exit(1);
    }

    if (end < 0 || end > strv.len) {
        fprintf(stderr,
                "fatal: invalid ending slice index %d for str of length %zu\n",
                end, strv.len);
        exit(1);
    }

    return (pf_strv){
        ._s = strv._s + (start * sizeof(char)),
        .len = end - start,
    };
}

pf_strv _pf_str_slice(pf_str str, int start, int end)
{ return _pf_strv_slice(PF_TOVIEW(str), start, end); }

pf_rune _pf_strv_index(pf_strv strv, int idx)
{
    if (idx < 0) {
        fprintf(stderr, "fatal: invalid negative index %d for str\n", idx);
        exit(1);
    }

    const char *p = PF_CSTRING(strv);
    int cps = 0;
    size_t bytes = 0;
    while (bytes < strv.len) {
        size_t len;
        pf_rune cp = decode_utf8((char *) p, &len);
        if (cps == idx)
            return cp;

        p += len * sizeof(char);
        bytes += len;
        cps += 1;
    }

    fprintf(stderr, "fatal: invalid index %d for str with %d codepoints\n", idx,
            cps);
    exit(1);
}

pf_rune _pf_str_index(pf_str str, int idx)
{ return _pf_strv_index(PF_TOVIEW(str), idx); }

char _pf_strv_index_byte(pf_strv strv, int idx)
{
    if (idx < 0 || idx >= strv.len) {
        fprintf(stderr, "fatal: invalid byte index %d for str of length %zu\n",
                idx, strv.len);
        exit(1);
    }

    return strv._s[idx];
}

char _pf_str_index_byte(pf_str str, int idx)
{ return _pf_strv_index_byte(PF_TOVIEW(str), idx); }

void _pf_strv_print(pf_strv strv)
{
    for (size_t i = 0; i < strv.len; i++)
        putchar(strv._s[i]);
}

void _pf_str_print(pf_str str)
{ _pf_strv_print(PF_TOVIEW(str)); }

void pf_rune_print(pf_rune rune)
{
    char buf[4];
    int len = encode_utf8(rune, buf);
    if (len != -1) {
        for (size_t i = 0; i < len; i++)
            putchar(buf[i]);
    } else {
        printf("<invalid-rune>");
    }
}

pf_str_iterator _pf_strv_iterator_new(pf_strv strv)
{
    pf_str_iterator_state state = _pf_str_iterator_iterating;
    if (strv.len == 0)
        state = _pf_str_iterator_done;
    return (pf_str_iterator){ ._strv = strv, ._state = state };
}

pf_str_iterator _pf_str_iterator_new(pf_str str)
{ return _pf_strv_iterator_new(PF_TOVIEW(str)); }

pf_rune pf_str_iterator_next(pf_str_iterator *it, size_t *len)
{
    pf_strv ret = it->_strv;
    pf_rune cp;
    size_t cplen;

    if (it->_state == _pf_str_iterator_iterating) {
        pf_strv strv = it->_strv;
        const char *p = PF_CSTRING(strv);
        cp = decode_utf8((char *) p, &cplen);
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
