#include "pf_str.h"

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
    return (pf_strv){
        ._s = buf,
        .len = len,
    };
}

pf_str pf_str_new_alloc(size_t len)
{
    char *buf = malloc(len + 1);  // for null terminator
    if (!buf) {
        fprintf(stderr, "fatal: out of memory!\n");
        abort();
    }
    buf[len] = '\0';
    return (pf_str){
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
        fprintf(stderr, "fatal: failed to format string\n");
        exit(1);
    }

    char *buf = malloc(len + 1);
    if (!buf) {
        va_end(args2);
        fprintf(stderr, "fatal: out of memory!\n");
        abort();
    }

    vsnprintf(buf, len + 1, fmt, args2);
    va_end(args2);

    return (pf_str){ ._s = buf, .len = len };
}

void pf_str_free(pf_str s)
{
    if (s._s == NULL)
        return;
    free(s._s);
    s._s = NULL;
}

// TODO: fix use after free when this is called with `b` aliasing `a`
void _pf_str_grow_with_strv(pf_str *a, pf_strv b)
{
    if (a->len > SIZE_MAX - b.len - 1) {
        fprintf(
            stderr,
            "fatal: attempt to grow a pf_str over size limit of %zu bytes\n",
            SIZE_MAX);
        exit(1);
    }

    size_t newlen = a->len + b.len;
    char *new = realloc(a->_s, newlen + 1);
    if (!new) {
        fprintf(stderr, "fatal: out of memory!\n");
        abort();
    }
    a->_s = new;

    memcpy(a->_s + a->len, b._s, b.len);
    a->_s[newlen] = '\0';
    a->len = newlen;
}

void _pf_str_grow_with_str(pf_str *a, pf_str b)
{ _pf_str_grow_with_strv(a, pf_STRTOVIEW(b)); }

pf_str _pf_strv_concat(pf_strv a, pf_strv b)
{
    if (a.len > SIZE_MAX - b.len - 1) {
        fprintf(stderr,
                "fatal: attempt to create a concatenated pf_str over "
                "size limit of %zu bytes\n",
                SIZE_MAX);
        exit(1);
    }

    size_t len = a.len + b.len;
    char *buf = malloc(len + 1);
    if (!buf) {
        fprintf(stderr, "fatal: out of memory!\n");
        abort();
    }

    memcpy(buf, a._s, a.len);
    memcpy(buf + a.len, b._s, b.len);
    buf[len] = '\0';
    return (pf_str){ ._s = buf, .len = len };
}

pf_str _pf_str_concat(pf_str a, pf_str b)
{ return _pf_strv_concat(pf_STRTOVIEW(a), pf_STRTOVIEW(b)); }

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
{ return _pf_strv_equal(pf_STRTOVIEW(a), pf_STRTOVIEW(b)); }

pf_strv _pf_strv_slice(pf_strv sv, size_t start, size_t end)
{
    if (start > end) {
        fprintf(stderr,
                "fatal: starting slice index %zu is larger than ending "
                "index %zu\n",
                start, end);
        exit(1);
    }

    if (start > sv.len) {
        fprintf(stderr,
                "fatal: invalid starting slice index %zu for s of length "
                "%zu\n",
                start, sv.len);
        exit(1);
    }

    if (end > sv.len) {
        fprintf(stderr,
                "fatal: invalid ending slice index %zu for s of length %zu\n",
                end, sv.len);
        exit(1);
    }

    return (pf_strv){
        ._s = sv._s + start,
        .len = end - start,
    };
}

pf_strv _pf_str_slice(pf_str s, size_t start, size_t end)
{ return _pf_strv_slice(pf_STRTOVIEW(s), start, end); }

pf_rune _pf_strv_index(pf_strv sv, size_t idx)
{
    const char *p = PF_CSTRING(sv);
    int cps = 0;
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

    fprintf(stderr, "fatal: invalid index %zu for s with %d codepoints\n", idx,
            cps);
    exit(1);
}

pf_rune _pf_str_index(pf_str s, size_t idx)
{ return _pf_strv_index(pf_STRTOVIEW(s), idx); }

char _pf_strv_index_byte(pf_strv sv, size_t idx)
{
    if (idx >= sv.len) {
        fprintf(stderr, "fatal: invalid byte index %zu for s of length %zu\n",
                idx, sv.len);
        exit(1);
    }

    return sv._s[idx];
}

char _pf_str_index_byte(pf_str s, size_t idx)
{ return _pf_strv_index_byte(pf_STRTOVIEW(s), idx); }

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
{ return _pf_strv_trim_left(pf_STRTOVIEW(s), cutset); }

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
{ return _pf_strv_trim_right(pf_STRTOVIEW(s), cutset); }

pf_strv _pf_strv_trim(pf_strv sv, const char *cutset)
{
    pf_strv left = _pf_strv_trim_left(sv, cutset);
    return _pf_strv_trim_right(left, cutset);
}

pf_strv _pf_str_trim(pf_str s, const char *cutset)
{ return _pf_strv_trim(pf_STRTOVIEW(s), cutset); }

void _pf_strv_print(pf_strv sv)
{
    for (size_t i = 0; i < sv.len; i++)
        putchar(sv._s[i]);
}

void _pf_str_print(pf_str s)
{ _pf_strv_print(pf_STRTOVIEW(s)); }

pf_str_iterator _pf_strv_iterator_new(pf_strv sv)
{
    _pf_str_iterator_state state = _pf_str_iterator_iterating;
    if (sv.len == 0)
        state = _pf_str_iterator_done;
    return (pf_str_iterator){ ._strv = sv, ._state = state };
}

pf_str_iterator _pf_str_iterator_new(pf_str s)
{ return _pf_strv_iterator_new(pf_STRTOVIEW(s)); }

pf_rune pf_str_iterator_next(pf_str_iterator *it, size_t *len)
{
    pf_rune cp;
    size_t cplen;

    if (it->_state == _pf_str_iterator_iterating) {
        pf_strv sv = it->_strv;
        const char *p = PF_CSTRING(sv);
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
