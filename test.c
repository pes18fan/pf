#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "greatest.h"
#include <sys/types.h>
#include <sys/wait.h>

#define PF_STR_IMPLEMENTATION
#include "pf_str.h"

// Extra testing stuff for testing by running a child process and observing what
// it outputs

typedef struct {
    int status;
    char out[512];
    size_t out_len;
} child_result;

static child_result run_child(void (*fn)(void))
{
    child_result res = { .status = -1, .out_len = 0 };
    int fds[2];

    if (pipe(fds) != 0) {
        perror("pipe");
        exit(2);
    }

    fflush(NULL);
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(2);
    }

    if (pid == 0) {
        close(fds[0]);
        if (dup2(fds[1], STDOUT_FILENO) < 0 || dup2(fds[1], STDERR_FILENO) < 0)
            _Exit(2);
        close(fds[1]);
        fn();
        fflush(NULL);
        _Exit(0);
    }

    close(fds[1]);
    ssize_t n;
    while ((n = read(fds[0], res.out + res.out_len,
                     sizeof res.out - 1 - res.out_len)) > 0)
        res.out_len += (size_t) n;
    close(fds[0]);

    int status = 0;
    waitpid(pid, &status, 0);
    res.status = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    res.out[res.out_len] = '\0';
    return res;
}

#define ASSERT_VIEW(EXPECTED, EXPECTED_LEN, GOT)                      \
    do {                                                              \
        ASSERT_EQ((size_t) (EXPECTED_LEN), (GOT).len);                \
        ASSERT_MEM_EQ((EXPECTED), (GOT)._s, (size_t) (EXPECTED_LEN)); \
    } while (0)

#define ASSERT_CHILD_OUTPUT(EXPECTED, EXPECTED_LEN, FN)             \
    do {                                                            \
        child_result _r = run_child(FN);                            \
        ASSERT_EQ(0, _r.status);                                    \
        ASSERT_EQ((size_t) (EXPECTED_LEN), _r.out_len);             \
        ASSERT_MEM_EQ((EXPECTED), _r.out, (size_t) (EXPECTED_LEN)); \
    } while (0)

#define ASSERT_CHILD_FATAL(FN)           \
    do {                                 \
        child_result _r = run_child(FN); \
        ASSERT_EQ(1, _r.status);         \
    } while (0)

static void child_slice_start_gt_end(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_slice(v, 2, 1);
}

static void child_slice_negative_start(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_slice(v, -1, 2);
}

static void child_slice_end_past_len(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_slice(v, 0, 4);
}

static void child_index_negative(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_index(v, -1);
}

static void child_index_out_of_range(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_index(v, 3);
}

static void child_index_empty(void)
{
    pf_strv v = PF_STRLIT("");
    (void) pf_str_index(v, 0);
}

static void child_index_byte_negative(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_index_byte(v, -1);
}

static void child_index_byte_out_of_range(void)
{
    pf_strv v = PF_STRLIT("abc");
    (void) pf_str_index_byte(v, 3);
}

static void child_print_all(void)
{
    pf_str_print(PF_STRLIT("hello"));

    pf_strv abcdef = PF_STRLIT("abcdef");
    pf_str_print(pf_str_slice(abcdef, 2, 4));

    pf_str_print(PF_STRLIT(""));
    pf_str_print(PF_STRLIT("a\0b"));
    pf_str_print(PF_STRLIT("\xC3\xA9\xE2\x82\xAC"));

    pf_str s = pf_str_new_alloc_formatted("hi");
    pf_str_print(s);
    pf_str_free(s);
}

static void child_rune_print_all(void)
{
    pf_rune_print('A');
    pf_rune_print(0xE9);
    pf_rune_print(0x20AC);
    pf_rune_print(0x1F600);
    pf_rune_print(0xD800);  // invalid codepoint
    pf_rune_print(0);
}

static void child_rune_print_boundaries(void)
{
    pf_rune_print(0x7FF);
    pf_rune_print(0x800);
    pf_rune_print(0xD7FF);
    pf_rune_print(0xE000);
    pf_rune_print(0xFFFF);
    pf_rune_print(0x10000);
    pf_rune_print(0x10FFFF);
}

// Tests ----------------------------------------------------------------------

TEST test_pf_runetoascii(void)
{
    ASSERT_EQ(0, PF_RUNETOASCII(0));
    ASSERT_EQ(1, PF_RUNETOASCII(1));
    ASSERT_EQ('A', PF_RUNETOASCII('A'));
    ASSERT_EQ(0x7F, PF_RUNETOASCII(0x7F));
    ASSERT_EQ((char) -1, PF_RUNETOASCII(0x80));
    ASSERT_EQ((char) -1, PF_RUNETOASCII(0xFF));
    ASSERT_EQ((char) -1, PF_RUNETOASCII(0x10FFFF));
    PASS();
}

TEST test_pf_cstring(void)
{
    pf_str s = pf_str_new_alloc_formatted("capture");
    pf_strv v = PF_STRLIT("view");

    ASSERT(PF_CSTRING(s) == s._s);
    ASSERT(PF_CSTRING(v) == v._s);
    ASSERT_STR_EQ("capture", PF_CSTRING(s));
    ASSERT_STR_EQ("view", PF_CSTRING(v));

    pf_str_free(s);
    PASS();
}

TEST test_pf_toview(void)
{
    pf_str s = pf_str_new_alloc_formatted("hello");
    pf_strv v = PF_TOVIEW(s);

    ASSERT(v._s == s._s);
    ASSERT_EQ(s.len, v.len);
    ASSERT_VIEW("hello", 5, v);

    pf_str_free(s);
    PASS();
}

TEST test_pf_strlit(void)
{
    pf_strv v = PF_STRLIT("hello");
    ASSERT_EQ((size_t) 5, v.len);
    ASSERT_VIEW("hello", 5, v);

    pf_strv empty = PF_STRLIT("");
    ASSERT_EQ((size_t) 0, empty.len);

    pf_strv embedded = PF_STRLIT("a\0b");
    ASSERT_EQ((size_t) 3, embedded.len);
    ASSERT_VIEW("a\0b", 3, embedded);

    pf_strv utf8 = PF_STRLIT("\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    ASSERT_EQ((size_t) 9, utf8.len);
    ASSERT_VIEW("\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80", 9, utf8);
    PASS();
}

TEST test_pf_strv_new_from_nullterminated_buf(void)
{
    const char *buf = "hello";
    pf_strv v = _pf_strv_new_from_nullterminated_buf(buf, 5);
    ASSERT(v._s == buf);
    ASSERT_VIEW("hello", 5, v);

    pf_strv shorter = _pf_strv_new_from_nullterminated_buf(buf, 2);
    ASSERT_VIEW("he", 2, shorter);

    pf_strv empty = _pf_strv_new_from_nullterminated_buf(buf, 0);
    ASSERT_EQ((size_t) 0, empty.len);
    PASS();
}

TEST test_pf_strv_new_from_buf(void)
{
    char buf[10] = "abcdefghi";
    pf_strv v = pf_strv_new_from_buf(buf, 3);

    ASSERT(v._s == buf);
    ASSERT_VIEW("abc", 3, v);
    ASSERT_EQ('\0', buf[3]);
    ASSERT_EQ('e', buf[4]);

    char empty_buf[4] = { 'x', 'y', 'z', 'w' };
    pf_strv empty = pf_strv_new_from_buf(empty_buf, 0);
    ASSERT_EQ((size_t) 0, empty.len);
    ASSERT_EQ('\0', empty_buf[0]);
    PASS();
}

TEST test_pf_str_new_alloc(void)
{
    pf_str s = pf_str_new_alloc(5);
    ASSERT_EQ((size_t) 5, s.len);
    ASSERT(s._s != NULL);
    ASSERT_EQ('\0', s._s[5]);

    memcpy(s._s, "hello", 5);
    s._s[5] = '\0';
    ASSERT_VIEW("hello", 5, PF_TOVIEW(s));
    ASSERT_STR_EQ("hello", PF_CSTRING(s));
    pf_str_free(s);

    pf_str empty = pf_str_new_alloc(0);
    ASSERT_EQ((size_t) 0, empty.len);
    ASSERT(empty._s != NULL);
    ASSERT_EQ('\0', empty._s[0]);
    pf_str_free(empty);
    PASS();
}

TEST test_pf_str_new_alloc_formatted(void)
{
    pf_str a = pf_str_new_alloc_formatted("Hello, %s!", "world");
    ASSERT_STR_EQ("Hello, world!", PF_CSTRING(a));
    pf_str_free(a);

    pf_str b = pf_str_new_alloc_formatted("%d %zu", -5, (size_t) 42);
    ASSERT_STR_EQ("-5 42", PF_CSTRING(b));
    pf_str_free(b);

    pf_str empty = pf_str_new_alloc_formatted("%s", "");
    ASSERT_EQ((size_t) 0, empty.len);
    ASSERT_STR_EQ("", PF_CSTRING(empty));
    pf_str_free(empty);

    pf_str nul = pf_str_new_alloc_formatted("%c", '\0');
    ASSERT_EQ((size_t) 1, nul.len);
    ASSERT_EQ('\0', nul._s[0]);
    ASSERT_EQ('\0', nul._s[1]);
    pf_str_free(nul);

    pf_str utf8 = pf_str_new_alloc_formatted("%s", "\xC3\xA9");
    ASSERT_VIEW("\xC3\xA9", 2, PF_TOVIEW(utf8));
    pf_str_free(utf8);

    pf_str big = pf_str_new_alloc_formatted("%0*d", 512, 7);
    ASSERT_EQ((size_t) 512, big.len);
    for (size_t i = 0; i + 1 < big.len; i++)
        ASSERT_EQ('0', big._s[i]);
    ASSERT_EQ('7', big._s[big.len - 1]);
    ASSERT_EQ('\0', big._s[big.len]);
    pf_str_free(big);
    PASS();
}

TEST test_pf_str_free(void)
{
    pf_str a = pf_str_new_alloc(0);
    pf_str b = pf_str_new_alloc_formatted("free me");
    pf_str empty = { ._s = NULL, .len = 0 };

    pf_str_free(a);
    pf_str_free(b);
    pf_str_free(empty);
    PASS();
}

TEST test_pf_str_grow(void)
{
    pf_str s = pf_str_new_alloc_formatted("abc");
    pf_str_grow(&s, PF_STRLIT("def"));
    ASSERT_STR_EQ("abcdef", PF_CSTRING(s));

    pf_str suffix = pf_str_new_alloc_formatted("ghi");
    pf_str_grow(&s, suffix);
    ASSERT_STR_EQ("abcdefghi", PF_CSTRING(s));
    pf_str_free(suffix);

    pf_str_grow(&s, PF_STRLIT(""));
    ASSERT_STR_EQ("abcdefghi", PF_CSTRING(s));
    pf_str_free(s);

    pf_str repeated = pf_str_new_alloc(0);
    for (int i = 0; i < 100; i++)
        pf_str_grow(&repeated, PF_STRLIT("ab"));
    ASSERT_EQ((size_t) 200, repeated.len);
    ASSERT_EQ('a', repeated._s[0]);
    ASSERT_EQ('b', repeated._s[199]);
    ASSERT_EQ('\0', repeated._s[200]);
    pf_str_free(repeated);

    pf_str utf8 = pf_str_new_alloc_formatted("%s", "\xC3\xA9");
    pf_str_grow(&utf8, PF_STRLIT("\xE2\x82\xAC"));
    ASSERT_VIEW("\xC3\xA9\xE2\x82\xAC", 5, PF_TOVIEW(utf8));
    pf_str_free(utf8);
    PASS();
}

TEST test_pf_str_concat(void)
{
    pf_str a = pf_str_concat(PF_STRLIT("foo"), PF_STRLIT("bar"));
    ASSERT_STR_EQ("foobar", PF_CSTRING(a));
    pf_str_free(a);

    pf_str lhs = pf_str_new_alloc_formatted("foo");
    pf_str rhs = pf_str_new_alloc_formatted("bar");
    pf_str b = pf_str_concat(lhs, rhs);
    ASSERT_STR_EQ("foobar", PF_CSTRING(b));
    pf_str_free(lhs);
    pf_str_free(rhs);
    pf_str_free(b);

    pf_str empty = pf_str_concat(PF_STRLIT(""), PF_STRLIT(""));
    ASSERT_EQ((size_t) 0, empty.len);
    ASSERT_STR_EQ("", PF_CSTRING(empty));
    pf_str_free(empty);

    pf_str left = pf_str_concat(PF_STRLIT("abc"), PF_STRLIT(""));
    ASSERT_STR_EQ("abc", PF_CSTRING(left));
    pf_str_free(left);

    pf_str right = pf_str_concat(PF_STRLIT(""), PF_STRLIT("abc"));
    ASSERT_STR_EQ("abc", PF_CSTRING(right));
    pf_str_free(right);

    char a1000[1001], b1000[1001];
    memset(a1000, 'a', 1000);
    a1000[1000] = '\0';
    memset(b1000, 'b', 1000);
    b1000[1000] = '\0';
    pf_strv av = { ._s = a1000, .len = 1000 };
    pf_strv bv = { ._s = b1000, .len = 1000 };
    pf_str big = pf_str_concat(av, bv);
    ASSERT_EQ((size_t) 2000, big.len);
    ASSERT_EQ('a', big._s[0]);
    ASSERT_EQ('a', big._s[999]);
    ASSERT_EQ('b', big._s[1000]);
    ASSERT_EQ('b', big._s[1999]);
    ASSERT_EQ('\0', big._s[2000]);
    pf_str_free(big);
    PASS();
}

TEST test_pf_str_equal(void)
{
    pf_strv abc = PF_STRLIT("abc");
    ASSERT(pf_str_equal(abc, PF_STRLIT("abc")));
    ASSERT_FALSE(pf_str_equal(abc, PF_STRLIT("abd")));
    ASSERT_FALSE(pf_str_equal(abc, PF_STRLIT("ab")));
    ASSERT_FALSE(pf_str_equal(PF_STRLIT("ab"), abc));
    ASSERT(pf_str_equal(PF_STRLIT(""), PF_STRLIT("")));

    pf_strv nul1 = PF_STRLIT("a\0b");
    pf_strv nul2 = PF_STRLIT("a\0b");
    pf_strv nul3 = PF_STRLIT("a\0c");
    ASSERT(pf_str_equal(nul1, nul2));
    ASSERT_FALSE(pf_str_equal(nul1, nul3));

    pf_str x = pf_str_new_alloc_formatted("abc");
    pf_str y = pf_str_new_alloc_formatted("abc");
    pf_str z = pf_str_new_alloc_formatted("abd");
    ASSERT(pf_str_equal(x, y));
    ASSERT_FALSE(pf_str_equal(x, z));
    ASSERT(pf_str_equal(PF_TOVIEW(x), abc));
    ASSERT_FALSE(pf_str_equal(PF_TOVIEW(x), PF_STRLIT("abd")));
    pf_str_free(x);
    pf_str_free(y);
    pf_str_free(z);
    PASS();
}

TEST test_pf_str_slice(void)
{
    pf_strv v = PF_STRLIT("abcdef");
    pf_strv whole = pf_str_slice(v, 0, 6);
    ASSERT(whole._s == v._s);
    ASSERT_VIEW("abcdef", 6, whole);

    pf_strv mid = pf_str_slice(v, 2, 4);
    ASSERT(mid._s == v._s + 2);
    ASSERT_VIEW("cd", 2, mid);

    pf_strv empty_start = pf_str_slice(v, 0, 0);
    ASSERT(empty_start._s == v._s);
    ASSERT_EQ((size_t) 0, empty_start.len);

    pf_strv empty_end = pf_str_slice(v, 6, 6);
    ASSERT(empty_end._s == v._s + 6);
    ASSERT_EQ((size_t) 0, empty_end.len);

    pf_str s = pf_str_new_alloc_formatted("abcdef");
    pf_strv str_mid = pf_str_slice(s, 2, 4);
    ASSERT(str_mid._s == s._s + 2);
    ASSERT_VIEW("cd", 2, str_mid);
    pf_str_free(s);

    ASSERT_CHILD_FATAL(child_slice_start_gt_end);
    ASSERT_CHILD_FATAL(child_slice_negative_start);
    ASSERT_CHILD_FATAL(child_slice_end_past_len);
    PASS();
}

TEST test_pf_str_trim(void)
{
    pf_strv v = PF_STRLIT("  hello  ");
    pf_strv left = pf_str_trim_left(v, " ");
    ASSERT(left._s == v._s + 2);
    ASSERT_VIEW("hello  ", 7, left);

    pf_strv right = pf_str_trim_right(v, " ");
    ASSERT(right._s == v._s);
    ASSERT_VIEW("  hello", 7, right);

    pf_strv both = pf_str_trim(v, " ");
    ASSERT(both._s == v._s + 2);
    ASSERT_VIEW("hello", 5, both);

    pf_strv cuts = PF_STRLIT("xxabyy");
    ASSERT_VIEW("abyy", 4, pf_str_trim_left(cuts, "xy"));
    ASSERT_VIEW("xxab", 4, pf_str_trim_right(cuts, "xy"));
    ASSERT_VIEW("ab", 2, pf_str_trim(cuts, "xy"));

    pf_strv all = PF_STRLIT("aaaa");
    ASSERT_EQ((size_t) 0, pf_str_trim_left(all, "a").len);
    ASSERT_EQ((size_t) 0, pf_str_trim_right(all, "a").len);
    ASSERT_EQ((size_t) 0, pf_str_trim(all, "a").len);

    pf_strv one = PF_STRLIT("a");
    ASSERT_EQ((size_t) 0, pf_str_trim_left(one, "a").len);
    ASSERT_EQ((size_t) 0, pf_str_trim_right(one, "a").len);
    ASSERT_EQ((size_t) 0, pf_str_trim(one, "a").len);

    pf_strv cuts2 = PF_STRLIT("baaa");
    ASSERT_VIEW("b", 1, pf_str_trim_right(cuts2, "a"));

    pf_strv empty_cutset = PF_STRLIT("  hi  ");
    ASSERT_VIEW("  hi  ", 6, pf_str_trim_left(empty_cutset, ""));
    ASSERT_VIEW("  hi  ", 6, pf_str_trim_right(empty_cutset, ""));
    ASSERT_VIEW("  hi  ", 6, pf_str_trim(empty_cutset, ""));

    pf_str s = pf_str_new_alloc_formatted("%s", "  hi  ");
    ASSERT_VIEW("hi  ", 4, pf_str_trim_left(s, " "));
    ASSERT_VIEW("  hi", 4, pf_str_trim_right(s, " "));
    pf_strv str_trimmed = pf_str_trim(s, " ");
    ASSERT(str_trimmed._s == s._s + 2);
    ASSERT_VIEW("hi", 2, str_trimmed);
    pf_str_free(s);

    // Views must be bounded by their length, not by the backing buffer.
    char buf[5] = "aaaa";
    pf_strv bounded = { ._s = buf, .len = 3 };
    pf_strv bounded_left = pf_str_trim_left(bounded, "a");
    ASSERT_EQ((size_t) 0, bounded_left.len);
    ASSERT(bounded_left._s == buf + 3);

    char non_nul[5] = "abcx";
    pf_strv non_nul_view = { ._s = non_nul, .len = 3 };
    ASSERT_VIEW("ab", 2, pf_str_trim_right(non_nul_view, "c"));
    ASSERT_VIEW("bc", 2, pf_str_trim_left(non_nul_view, "a"));
    ASSERT_VIEW("b", 1, pf_str_trim(non_nul_view, "ac"));
    PASS();
}

TEST test_pf_str_index(void)
{
    pf_strv abc = PF_STRLIT("abc");
    ASSERT_EQ('a', pf_str_index(abc, 0));
    ASSERT_EQ('b', pf_str_index(abc, 1));
    ASSERT_EQ('c', pf_str_index(abc, 2));

    pf_str s = pf_str_new_alloc_formatted("abc");
    ASSERT_EQ('a', pf_str_index(s, 0));
    ASSERT_EQ('c', pf_str_index(s, 2));
    pf_str_free(s);

    pf_strv utf8 = PF_STRLIT("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    ASSERT_EQ(0x61, pf_str_index(utf8, 0));
    ASSERT_EQ(0xE9, pf_str_index(utf8, 1));
    ASSERT_EQ(0x20AC, pf_str_index(utf8, 2));
    ASSERT_EQ(0x1F600, pf_str_index(utf8, 3));
    ASSERT_EQ(0x10FFFF, pf_str_index(PF_STRLIT("\xF4\x8F\xBF\xBF"), 0));

    pf_strv sliced = pf_str_slice(PF_STRLIT("abcdef"), 2, 4);
    ASSERT_EQ('c', pf_str_index(sliced, 0));
    ASSERT_EQ('d', pf_str_index(sliced, 1));

    pf_strv nul = PF_STRLIT("a\0b");
    ASSERT_EQ('a', pf_str_index(nul, 0));
    ASSERT_EQ(0, pf_str_index(nul, 1));
    ASSERT_EQ('b', pf_str_index(nul, 2));

    ASSERT_CHILD_FATAL(child_index_negative);
    ASSERT_CHILD_FATAL(child_index_out_of_range);
    ASSERT_CHILD_FATAL(child_index_empty);
    PASS();
}

TEST test_pf_str_index_byte(void)
{
    pf_strv abc = PF_STRLIT("abc");
    ASSERT_EQ('a', pf_str_index_byte(abc, 0));
    ASSERT_EQ('b', pf_str_index_byte(abc, 1));
    ASSERT_EQ('c', pf_str_index_byte(abc, 2));

    pf_str s = pf_str_new_alloc_formatted("abc");
    ASSERT_EQ('a', pf_str_index_byte(s, 0));
    ASSERT_EQ('c', pf_str_index_byte(s, 2));
    pf_str_free(s);

    pf_strv utf8 = PF_STRLIT("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80");
    static const unsigned char bytes[] = {
        'a', 0xC3, 0xA9, 0xE2, 0x82, 0xAC, 0xF0, 0x9F, 0x98, 0x80,
    };
    for (size_t i = 0; i < sizeof bytes; i++)
        ASSERT_EQ(bytes[i], (unsigned char) pf_str_index_byte(utf8, (int) i));

    pf_strv sliced = pf_str_slice(PF_STRLIT("abc"), 1, 3);
    ASSERT_EQ('b', pf_str_index_byte(sliced, 0));
    ASSERT_EQ('c', pf_str_index_byte(sliced, 1));

    pf_strv nul = PF_STRLIT("a\0b");
    ASSERT_EQ(0, pf_str_index_byte(nul, 1));

    ASSERT_CHILD_FATAL(child_index_byte_negative);
    ASSERT_CHILD_FATAL(child_index_byte_out_of_range);
    PASS();
}

TEST test_pf_str_print(void)
{
    static const char expected[] = "hellocda\0b\xC3\xA9\xE2\x82\xAChi";
    ASSERT_CHILD_OUTPUT(expected, sizeof expected - 1, child_print_all);
    PASS();
}

TEST test_pf_rune_print(void)
{
    static const char expected[] =
        "A\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80<invalid-rune>\0";
    ASSERT_CHILD_OUTPUT(expected, sizeof expected - 1, child_rune_print_all);

    static const char boundaries[] =
        "\xDF\xBF\xE0\xA0\x80\xED\x9F\xBF\xEE\x80\x80\xEF\xBF\xBF"
        "\xF0\x90\x80\x80\xF4\x8F\xBF\xBF";
    ASSERT_CHILD_OUTPUT(boundaries, sizeof boundaries - 1,
                        child_rune_print_boundaries);
    PASS();
}

TEST test_pf_str_iterator_new(void)
{
    pf_strv empty = PF_STRLIT("");
    pf_str_iterator empty_it = pf_str_iterator_new(empty);
    ASSERT(pf_str_iterator_is_done(empty_it));

    pf_strv abc = PF_STRLIT("abc");
    pf_str_iterator abc_it = pf_str_iterator_new(abc);
    ASSERT_FALSE(pf_str_iterator_is_done(abc_it));

    pf_str s = pf_str_new_alloc_formatted("abc");
    pf_str_iterator str_it = pf_str_iterator_new(s);
    ASSERT_FALSE(pf_str_iterator_is_done(str_it));
    pf_str_free(s);
    PASS();
}

TEST test_pf_str_iterator_next(void)
{
    pf_str_iterator ascii = pf_str_iterator_new(PF_STRLIT("abc"));
    static const pf_rune ascii_runes[] = { 'a', 'b', 'c' };
    for (size_t i = 0; i < 3; i++) {
        size_t len = 0;
        ASSERT_EQ(ascii_runes[i], pf_str_iterator_next(&ascii, &len));
        ASSERT_EQ((size_t) 1, len);
    }
    ASSERT(pf_str_iterator_is_done(ascii));

    size_t len = 99;
    ASSERT_EQ(0, pf_str_iterator_next(&ascii, &len));
    ASSERT_EQ((size_t) 0, len);

    pf_str_iterator utf8 =
        pf_str_iterator_new(PF_STRLIT("a\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80"));
    static const pf_rune utf8_runes[] = { 0x61, 0xE9, 0x20AC, 0x1F600 };
    static const size_t utf8_lens[] = { 1, 2, 3, 4 };
    for (size_t i = 0; i < 4; i++) {
        size_t n = 0;
        ASSERT_EQ(utf8_runes[i], pf_str_iterator_next(&utf8, &n));
        ASSERT_EQ(utf8_lens[i], n);
    }
    ASSERT(pf_str_iterator_is_done(utf8));

    pf_str_iterator nul = pf_str_iterator_new(PF_STRLIT("a\0b"));
    ASSERT_EQ('a', pf_str_iterator_next(&nul, NULL));
    ASSERT_EQ(0, pf_str_iterator_next(&nul, NULL));
    ASSERT_EQ('b', pf_str_iterator_next(&nul, NULL));
    ASSERT(pf_str_iterator_is_done(nul));

    pf_str s = pf_str_new_alloc_formatted("%s", "\xC3\xA9");
    pf_str_iterator str_it = pf_str_iterator_new(s);
    size_t str_len = 0;
    ASSERT_EQ(0xE9, pf_str_iterator_next(&str_it, &str_len));
    ASSERT_EQ((size_t) 2, str_len);
    ASSERT(pf_str_iterator_is_done(str_it));
    pf_str_free(s);
    PASS();
}

TEST test_pf_str_iterator_is_done(void)
{
    pf_str_iterator empty = pf_str_iterator_new(PF_STRLIT(""));
    ASSERT(pf_str_iterator_is_done(empty));

    pf_str_iterator it = pf_str_iterator_new(PF_STRLIT("abc"));
    ASSERT_FALSE(pf_str_iterator_is_done(it));
    while (!pf_str_iterator_is_done(it))
        (void) pf_str_iterator_next(&it, NULL);
    ASSERT(pf_str_iterator_is_done(it));

    pf_str_iterator original = pf_str_iterator_new(PF_STRLIT("abc"));
    pf_str_iterator copy = original;
    while (!pf_str_iterator_is_done(copy))
        (void) pf_str_iterator_next(&copy, NULL);
    ASSERT_FALSE(pf_str_iterator_is_done(original));
    PASS();
}

SUITE(pf_str_suite)
{
    RUN_TEST(test_pf_runetoascii);
    RUN_TEST(test_pf_cstring);
    RUN_TEST(test_pf_toview);
    RUN_TEST(test_pf_strlit);
    RUN_TEST(test_pf_strv_new_from_nullterminated_buf);
    RUN_TEST(test_pf_strv_new_from_buf);
    RUN_TEST(test_pf_str_new_alloc);
    RUN_TEST(test_pf_str_new_alloc_formatted);
    RUN_TEST(test_pf_str_free);
    RUN_TEST(test_pf_str_grow);
    RUN_TEST(test_pf_str_concat);
    RUN_TEST(test_pf_str_equal);
    RUN_TEST(test_pf_str_slice);
    RUN_TEST(test_pf_str_trim);
    RUN_TEST(test_pf_str_index);
    RUN_TEST(test_pf_str_index_byte);
    RUN_TEST(test_pf_str_print);
    RUN_TEST(test_pf_rune_print);
    RUN_TEST(test_pf_str_iterator_new);
    RUN_TEST(test_pf_str_iterator_next);
    RUN_TEST(test_pf_str_iterator_is_done);
}

GREATEST_MAIN_DEFS();

int main(int argc, char **argv)
{
    GREATEST_MAIN_BEGIN();
    RUN_SUITE(pf_str_suite);
    GREATEST_MAIN_END();
}
