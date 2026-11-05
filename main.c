// Just a showcase
// Build the headers using `x` then try building and running this!
#include <stdbool.h>
#include <stdio.h>

#define PF_STR_IMPLEMENTATION
#include "pf_str.h"

void showcase_pf_str(void)
{
    puts("---- PF_STR ----");

    pf_str s = pf_str_new_alloc_formatted("Hello, %s!", "संसार");
    puts(PF_CSTRING(s));
    pf_str_iterator it = pf_str_iterator_new(s);
    puts("each codepoint:");
    while (!pf_str_iterator_is_done(it)) {
        size_t len;
        pf_rune next = pf_str_iterator_next(&it, &len);
        printf("\tlen: %zu, cp: %d, printed: ", len, next);
        pf_rune_print(next);
        putchar('\n');
    }

    pf_strv s2 = PF_STRLIT("Hallo, Welt!");
    bool equal = pf_str_equal(PF_TOVIEW(s), s2);
    printf("equal? %s\n", equal ? "yay" : "nay");

    pf_str_free(s);
}

#define PF_AP_IMPLEMENTATION
#include "pf_ap.h"

void showcase_pf_ap(int argc, const char *argv[])
{
    puts("---- PF_AP ----");

    pf_ap *p =
        pf_ap_make(NULL, argv[0], "little test", pf_ap_arg_none, NULL, NULL);

    pf_ap_flag_add(p, "--test", "-t", "just a test", pf_ap_arg_none, NULL,
                   NULL);
    pf_ap_flag_add(p, "--test2", "-d", "just another test", pf_ap_arg_none,
                   NULL, NULL);

    char *a;
    pf_ap *another =
        pf_ap_make(p, "hewwo", "another test", pf_ap_arg_str, &a, "a string");
    pf_ap_flag_add(another, "--heh", "-j", "just another another test",
                   pf_ap_arg_none, NULL, NULL);

    int parsed = pf_ap_parse(p, argc, argv);
    pf_ap_destroy(p);

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
