// Just a showcase
#include <stdbool.h>
#include <stdio.h>

#define PF_IMPLEMENTATION
#include "pf.h"

void showcase_pf_str(void)
{
    puts("---- PF_STR ----");

    str s = str_new_alloc_formatted("Hello, %s!", "संसार");
    puts(str_to_cstring(s));
    str_iterator it = str_iterator_new(s);
    puts("each codepoint:");
    while (!str_iterator_is_done(it)) {
        size_t len;
        rune next = str_iterator_next(&it, &len);
        printf("\tlen: %zu, cp: %d, printed: ", len, next);
        char cps[4];
        int n = rune_encode(next, cps);
        if (n == -1) {
            printf("<invalid>");
        } else {
            printf("%.*s", n, cps);
        }
        putchar('\n');
    }

    strv s2 = STRLIT("Hallo, Welt!");
    bool equal = str_equal(STRTOVIEW(s), s2);
    printf("equal? %s\n", equal ? "yay" : "nay");

    str_free(s);
}

void showcase_pf_arg(int argc, const char *argv[])
{
    puts("---- PF_ARG ----");

    argparser *p = argparser_make(NULL, argv[0], "little test",
                                  argparser_arg_none, NULL, NULL);

    argparser_flag_add(p, "--test", "-t", "just a test", argparser_arg_none,
                       NULL, NULL);
    argparser_flag_add(p, "--test2", "-d", "just another test",
                       argparser_arg_none, NULL, NULL);

    const char *a;
    argparser *another = argparser_make(p, "hewwo", "another test",
                                        argparser_arg_str, &a, "a string");
    argparser_flag_add(another, "--heh", "-j", "just another another test",
                       argparser_arg_none, NULL, NULL);

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
    showcase_pf_arg(argc, argv);
}
