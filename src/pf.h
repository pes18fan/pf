#ifndef PF_H
#define PF_H

#if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#error C11 support required
#endif

#ifndef PF_NO_SHORT_NAMES
#define panic pf_panic
#define panicf pf_panicf

#define fatal pf_fatal

#define ensure pf_ensure
#define ensuref pf_ensuref
#endif  // PF_NO_SHORT_NAMES

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>

// End the program with a certain error message.
// If really_bad is true, the program aborts instead of exiting and avoids
// writing formatted output.
static noreturn void panic_impl(const char *prefix, bool really_bad,
                                const char *fmt, ...)
{
    if (really_bad) {
        fputs(prefix, stderr);
        fputc(':', stderr);
        fputc(' ', stderr);
        fputs(fmt, stderr);
        fputc('\n', stderr);
        abort();
    } else {
        va_list list;
        va_start(list, fmt);
        fprintf(stderr, "%s: ", prefix);
        vfprintf(stderr, fmt, list);
        putc('\n', stderr);
        va_end(list);
        exit(1);
    }
}

#define pf_panic(message) panic_impl("panic", false, (message))
#define pf_panicf(fmt, ...) panic_impl("panic", false, (fmt), __VA_ARGS__)

// There is no corresponding fatalf, we can't be formatting strings when
// we're out of memory or something
#define pf_fatal(message) panic_impl("fatal", true, (message))

// Assertion
#define pf_ensure(expr, message)                                 \
    do {                                                         \
        if (!(expr)) {                                           \
            panic_impl("unsatisfied ensure:", false, (message)); \
        }                                                        \
    } while (0)

// Assertion with formatted errors
#define pf_ensuref(expr, message, ...)                                         \
    do {                                                                       \
        if (!(expr)) {                                                         \
            panic_impl("unsatisfied ensure: ", false, (message), __VA_ARGS__); \
        }                                                                      \
    } while (0)

#endif  // PF_H
