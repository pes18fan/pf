// Just a showcase
#include <stdbool.h>
#include <stdio.h>

#define PF_IMPLEMENTATION
#include "pf.h"

void showcase_pf_str(void)
{
    puts("---- PF_STR ----");

    str_t s = str_new_alloc_formatted("Hello, %s!", "संसार");
    puts(str_to_cstring(s));
    str_iterator_t it = str_iterator_new(s);
    puts("each codepoint:");
    while (!str_iterator_is_done(it)) {
        size_t len;
        rune_t next = str_iterator_next(&it, &len);
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

    strv_t s2 = STRLIT("Hallo, Welt!");
    bool equal = str_equal(STRTOVIEW(s), s2);
    printf("equal? %s\n", equal ? "yay" : "nay");

    str_free(s);
}

void showcase_pf_list(void)
{
    puts("---- PF_LIST ----");

    // A list of int64s
    list_t(int64_t) l1 = list_init(int64_t);
    printf("int64_t list:\n");
    list_append(l1, 14);
    list_append(l1, 25);
    list_append(l1, 36);
    list_append(l1, 47);
    list_append(l1, 58);
    list_append(l1, 69);
    list_append(l1, 72);
    list_append(l1, 83);
    list_append(l1, 94);
    list_append(l1, 6);
    printf("    len: %zu\n", list_len(l1));
    printf("    cap: %zu\n", list_cap(l1));
    printf("    items:\n");
    for (size_t i = 0; i < list_len(l1); i++) {
        // You can use l1[i] too, but list_get is bounds checked and will panic
        // if `i` is invalid.
        int64_t item = list_get(l1, i);
        printf("        %ld\n", item);
    }
    printf("    popped one: %ld\n", list_pop(l1));
    printf("    popped another: %ld\n", list_pop(l1));
    printf("    current last item: %ld\n", l1[list_len(l1) - 1]);
    list_free(l1);

    // A list of string views
    list_t(pf_strv_t) l2;
    printf("string view list:\n");
    l2 = list_init(pf_strv_t);
    list_append(l2, STRLIT("a"));
    list_append(l2, STRLIT("string"));
    list_append(l2, STRLIT("another"));
    list_append(l2, STRLIT("string"));
    list_append(l2, STRLIT("yet"));
    list_append(l2, STRLIT("another"));
    list_append(l2, STRLIT("little"));
    list_append(l2, STRLIT("string"));
    list_append(l2, STRLIT("here"));
    list_append(l2, STRLIT("example"));
    printf("    len: %zu\n", list_len(l2));
    printf("    cap: %zu\n", list_cap(l2));
    for (size_t i = 0; i < list_len(l2); i++) {
        printf("        ");
        str_print(l2[i]);
        putchar('\n');
    }
    printf("    popped one: ");
    str_print(list_pop(l2));
    putchar('\n');
    printf("    popped another: ");
    str_print(list_pop(l2));
    putchar('\n');
    printf("    current last item: ");
    str_print(l2[list_len(l2) - 1]);
    putchar('\n');
    list_free(l2);
}

#define PF_ARGPARSE_IMPLEMENTATION
#include "pf_argparse.h"

void showcase_pf_arg(void)
{
    puts("---- PF_ARG ----");

    const char *argv[] = { "./program", "-t", "hewwo", "abcd" };
    int argc = 4;

    argparser_t *p = argparser_make(NULL, argv[0], "little test",
                                    argparser_arg_none, NULL, NULL);

    argparser_flag_add(p, "--test", "-t", "just a test", argparser_arg_none,
                       NULL, NULL);
    argparser_flag_add(p, "--test2", "-d", "just another test",
                       argparser_arg_none, NULL, NULL);

    const char *a;
    argparser_t *another = argparser_make(p, "hewwo", "another test",
                                          argparser_arg_str, &a, "a string");
    argparser_flag_add(another, "--heh", "-j", "just another another test",
                       argparser_arg_none, NULL, NULL);

    int parsed = argparse(p, argc, argv);
    argparser_destroy(p);

    if (parsed == -1) {
        puts("failed to parse args!");
    } else {
        printf("got: %s\n", a);
    }
}

#define PF_ALLOCATOR_IMPLEMENTATION
#include "pf_allocator.h"

void showcase_pf_allocator(void)
{
    puts("---- PF_ALLOCATOR ----");

    arena_allocator_t arena;
    uint8_t buf[1024];
    arena_allocator_init(&arena, buf, sizeof buf);
    allocator_t alloc = pf_arena_allocator(&arena);

    char *mem = mem_alloc(&alloc, 10);
    strv_t str = STRLIT("henlo");
    for (size_t i = 0; i < str.len; i++) {
        mem[i] = str_index_byte(str, i);
        printf("we have: ");
        putchar(mem[i]);
        putchar('\n');
    }

    mem_free_all(&alloc);
}

int main(void)
{
    showcase_pf_str();
    showcase_pf_list();
    showcase_pf_arg();
    showcase_pf_allocator();
}
