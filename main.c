// Just a showcase
// Build the headers using `x` then try building and running this!
#include <stdbool.h>
#include <stdio.h>

#define PF_STR_IMPLEMENTATION
#include "pf_str.h"

void showcase_pf_str(void)
{
    puts("---- PF_STR ----");

    str s = str_new_alloc_formatted("Hello, %s!", "संसार");
    puts(CSTRING(s));
    str_iterator it = str_iterator_new(s);
    puts("each codepoint:");
    while (!str_iterator_is_done(it)) {
        size_t len;
        rune next = str_iterator_next(&it, &len);
        printf("\tlen: %zu, cp: %d, printed: ", len, next);
        rune_print(next);
        putchar('\n');
    }

    strv s2 = STRLIT("Hallo, Welt!");
    bool equal = str_equal(STRTOVIEW(s), s2);
    printf("equal? %s\n", equal ? "yay" : "nay");

    str_free(s);
}

#define PF_AP_IMPLEMENTATION
#include "pf_ap.h"

void showcase_pf_ap(int argc, const char *argv[])
{
    puts("---- PF_AP ----");

    argparser *p =
        argparser_make(NULL, argv[0], "little test", arg_none, NULL, NULL);

    flag_add(p, "--test", "-t", "just a test", arg_none, NULL, NULL);
    flag_add(p, "--test2", "-d", "just another test", arg_none, NULL, NULL);

    char *a;
    argparser *another =
        argparser_make(p, "hewwo", "another test", arg_str, &a, "a string");
    flag_add(another, "--heh", "-j", "just another another test", arg_none,
             NULL, NULL);

    int parsed = argparse(p, argc, argv);
    argparser_destroy(p);

    if (parsed == -1) {
        puts("failed to parse args!");
    } else {
        printf("got: %s", a);
    }
}

int main(int argc, const char *argv[])
{
    showcase_pf_str();
    showcase_pf_ap(argc, argv);
}
