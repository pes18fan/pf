// Just a showcase
#include <stdbool.h>
#include <stdio.h>

#define PF_STR_IMPLEMENTATION
#include "pf_str.h"

void showcase_pf_str(void)
{
    pf_str s = pf_str_new_alloc_formatted("Hello, %s!", "संसार");
    puts(PF_CSTRING(s));
    pf_str_iterator it = pf_str_iterator_new(s);
    printf("each codepoint:\n");
    while (!pf_str_iterator_is_done(it)) {
        size_t len;
        pf_rune next = pf_str_iterator_next(&it, &len);
        printf("\tlen: %zu, cp: %d, printed: ", len, next);
        pf_rune_print(next);
        putchar('\n');
    }

    pf_strv s2 = PF_STRLIT("Hello, Welt!");
    bool equal = pf_str_equal(PF_TOVIEW(s), s2);
    printf("equal? %s", equal ? "yay" : "nay");

    pf_str_free(s);
}

int main(void)
{ showcase_pf_str(); }
