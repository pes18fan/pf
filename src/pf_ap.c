#include "pf_ap.h"

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdnoreturn.h>
#include <string.h>

static noreturn void panic_impl(const char *prefix, const char *fmt, ...)
{
    va_list list;
    va_start(list, fmt);
    fprintf(stderr, "%s: ", prefix);
    vfprintf(stderr, fmt, list);
    putc('\n', stderr);
    va_end(list);
    exit(1);
}

// Simple wrapper for panic with formatted errors
#define panic(message) panic_impl("panic", message)

// Simple wrapper for panic with formatted errors
#define panicf(fmt, ...) panic_impl("panic", fmt, __VA_ARGS__)

// Assertion
#define ensure(expr, message)                           \
    do {                                                \
        if (!(expr)) {                                  \
            panic_impl("failed assertion:", (message)); \
        }                                               \
    } while (0)

// Assertion with formatted errors
#define ensuref(expr, message, ...)                                   \
    do {                                                              \
        if (!(expr)) {                                                \
            panic_impl("failed assertion: ", (message), __VA_ARGS__); \
        }                                                             \
    } while (0)

static inline const char *pfa_argument_kind_str(pfa_arg_kind k)
{
    switch (k) {
    case pfa_arg_none:
        return "none";
    case pfa_arg_int:
        return "int";
    case pfa_arg_boolean:
        return "boolean";
    case pfa_arg_str:
        return "string";
    }

    panicf("invalid argument kind str %d", k);
}

pfa_argparser *pfa_argparser_make(pfa_argparser *parent, const char *name,
                                  const char *description, pfa_arg_kind kind,
                                  void *arg_ptr, const char *arg_description)
{
    ensure(name != NULL, "parser_make: parser name cannot be NULL");

    if (parent != NULL) {
        // Ensure we're not adding too many subparsers
        ensuref(parent->subparser_count < MAX_SUBPARSERS,
                "parser_make: failed to add subparser %s to parent %s, "
                "parent has too many subparsers; only upto %d supported",
                name, parent->name, MAX_SUBPARSERS);

        // A parser cannot have a subparser if it takes an argument
        ensuref(parent->arg.kind == pfa_arg_none,
                "parser_make: cannot add subparser %s to parser %s because the "
                "latter takes a %s argument",
                name, parent->name, pfa_argument_kind_str(parent->arg.kind));

        // Ensure no duplicates
        for (int i = 0; i < parent->subparser_count; i++) {
            ensuref(strcmp(name, parent->subparsers[i]->name) != 0,
                    "parser_make: cannot add subcommand %s twice to parent %s",
                    name, parent->name);
        }
    }

    pfa_argparser *p = malloc(sizeof(pfa_argparser));
    ensuref(p != NULL, "parser_make: fatal error: %s", strerror(errno));

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case pfa_arg_int:
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case pfa_arg_str:
        *((const char **) arg_ptr) = "";
        break;
    case pfa_arg_boolean:
        *((bool *) arg_ptr) = false;
        break;
    case pfa_arg_none:
        break;  // nothing to do
    default:
        panicf("invalid argument kind %d", kind);
    }

    *p = (pfa_argparser){
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
    pfa_flag_add(p, "--help", "-h", "Display help message", pfa_arg_none, NULL,
                 NULL);

    if (parent != NULL) {
        parent->subparsers[parent->subparser_count] = p;
        parent->subparser_count++;
    }

    return p;
}

void pfa_argparser_destroy(pfa_argparser *parent)
{
    for (int i = 0; i < parent->subparser_count; i++) {
        pfa_argparser_destroy(parent->subparsers[i]);
    }

    for (int i = 0; i < parent->flag_count; i++) {
        free(parent->flags[i]);
    }

    free(parent);
}

void pfa_flag_add(pfa_argparser *parser, const char *name,
                  const char *short_name, const char *description,
                  pfa_arg_kind kind, void *arg_ptr, const char *arg_description)
{
    ensure(name != NULL, "flag_add: long name of flag cannot be NULL");

    // Ensure not too many flags
    ensuref(parser->flag_count < MAX_FLAGS,
            "flag_add: failed to add flag %s to parser %s, "
            "parser has too many flags; only upto %d supported",
            name, parser->name, MAX_FLAGS);

    // Flag must begin with --
    ensuref(strncmp(name, "--", 2) == 0,
            "flag_add: invalid flag name %s, it must begin with `--`", name);

    if (short_name != NULL) {
        // Short flag must begin with -
        ensuref(strncmp(short_name, "-", 1) == 0,
                "flag_add: invalid short flag name %s, it must begin with `-`",
                name);

        // Short flag must be exactly two chars long
        ensuref(strlen(short_name) == 2,
                "flag_add: invalid short flag name %s, it must be "
                "exactly two characters long",
                short_name);
    }

    // Ensure no duplicates, and that the --help or -h flags are not duplicated.
    // The latter is the same as the duplicate problem, but special cased to
    // provide a clearer error message.
    for (int i = 0; i < parser->flag_count; i++) {
        ensuref(strcmp(name, parser->flags[i]->name) != 0,
                "flag_add: cannot add flag %s twice", name);

        if (short_name != NULL && parser->flags[i]->short_name != NULL) {
            ensuref(strcmp(short_name, parser->flags[i]->short_name) != 0,
                    "flag_add: cannot add flag with short name %s twice",
                    short_name);
        }
    }

    pfa_flag *f = malloc(sizeof(pfa_flag));
    ensuref(f != NULL, "flag_add: fatal error: %s", strerror(errno));

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case pfa_arg_int:
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case pfa_arg_str:
        *((const char **) arg_ptr) = "";
        break;
    case pfa_arg_boolean:
        *((bool *) arg_ptr) = false;
        break;
    case pfa_arg_none:
        break;  // nothing to do
    default:
        panicf("invalid argument kind %d", kind);
    }

    *f = (pfa_flag){
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
void pfa_print_usage(pfa_argparser *parser)
{
    printf("%s", parser->name);
    if (parser->description != NULL) {
        printf(": %s", parser->description);
    }
    printf("\n\n");

    printf("Usage:\n");
    printf("    ");
    if (parser->arg.kind == pfa_arg_none) {
        printf("%s", parser->name);

        if (parser->subparser_count > 0) {
            printf(" <subcommand>");
        }

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }
    } else {
        printf("%s <%s>", parser->name,
               pfa_argument_kind_str(parser->arg.kind));

        // No subparser_count check here, as a parser that takes an argument
        // is NOT allowed to have any subcommands

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }

        if (parser->arg.description != NULL) {
            printf("\n\n");
            printf("Argument of type %s: %s",
                   pfa_argument_kind_str(parser->arg.kind),
                   parser->arg.description);
        }
    }
    printf("\n\n");

    printf("Subcommands:\n");
    if (parser->subparser_count == 0) {
        printf("    None\n");
    } else {
        for (int i = 0; i < parser->subparser_count; i++) {
            pfa_argparser *subparser = parser->subparsers[i];

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
            pfa_flag *flag = parser->flags[i];

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
static int pfa_parse_argument(pfa_argparser *parser, int argc,
                              const char *argv[])
{
    if (argc == 0 && parser->arg.kind != pfa_arg_none &&
        parser->arg.kind != pfa_arg_boolean) {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pfa_argument_kind_str(parser->arg.kind), parser->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (parser->arg.kind) {
    case pfa_arg_none:
        break;
    case pfa_arg_boolean:
        *((bool *) parser->arg._value) = true;
        break;
    case pfa_arg_int: {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            panicf("parse: integer overflow on %ld", v);

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
    case pfa_arg_str: {
        const char *arg = argv[0];
        *((const char **) parser->arg._value) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        panicf("parse: invalid argument kind %d", parser->arg.kind);
    }

    return initial_argc - argc;
}

// Attempt to parse an argument. It is considered an error if no valid argument
// is found.
// Returns the number of arguments parsed on success, and -1 on error.
static int pfa_parse_argument_flag(pfa_flag *flag, int argc, const char *argv[])
{
    if (argc == 0 && flag->arg.kind != pfa_arg_none &&
        flag->arg.kind != pfa_arg_boolean) {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pfa_argument_kind_str(flag->arg.kind), flag->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (flag->arg.kind) {
    case pfa_arg_none:
        break;
    case pfa_arg_boolean:
        *((bool *) flag->arg._value) = true;
        break;
    case pfa_arg_int: {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            panicf("parse: integer overflow on %ld", v);

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
    case pfa_arg_str: {
        const char *arg = argv[0];
        *((const char **) flag->arg._value) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        panicf("parse: invalid argument kind %d", flag->arg.kind);
    }

    return initial_argc - argc;
}

// Parse flags if found. If no args are remaining it doesn't do anything.
// Returns a whole number (0 or higher) on success, which is the number of flags
// parsed. Returns -1 on error.
static int pfa_parse_flags(pfa_argparser *parser, int argc, const char *argv[])
{
    if (argc == 0)
        return 0;

    int initial_argc = argc;

    // If we find a flag, start parsing them
    while (argc > 0 &&
           (strncmp(argv[0], "--", 2) == 0 || strncmp(argv[0], "-", 1) == 0)) {
        const char *flag_name = argv[0];
        pfa_flag *received_flag = NULL;

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
            pfa_print_usage(parser);
            return -1;
        }

        // If we received the help flag just print usage and exit
        if (strcmp(received_flag->name, "--help") == 0 ||
            (received_flag->short_name != NULL &&
             strcmp(received_flag->short_name, "-h") == 0)) {
            pfa_print_usage(parser);
            exit(0);
        }

        argv++;
        argc--;

        // Try to parse argument
        int skipped = pfa_parse_argument_flag(received_flag, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    return initial_argc - argc;
}

int pfa_argparse(pfa_argparser *parser, int argc, const char *argv[])
{
    int initial_argc = argc;
    int skipped;

    ensure(argc >= 1,
           "parse: invalid arguments provided, need at least 1 arg\n");

    // Consume the first arg (program / subcommand name)
    argv++;
    argc--;

    // Try to parse flags
    skipped = pfa_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // If the parser has any subparsers, parse them recursively
    if (argc > 0 && parser->subparser_count > 0) {
        const char *subcommand_name = argv[0];
        pfa_argparser *received_subparser = NULL;
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

        skipped = pfa_argparse(received_subparser, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    // Try to parse flags... again
    skipped = pfa_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // Try to parse argument
    skipped = pfa_parse_argument(parser, argc, argv);
    if (skipped == -1)
        goto die;

    argv += skipped;
    argc -= skipped;

    // Parse any remaining flags...
    skipped = pfa_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    return initial_argc - argc;

die:
    pfa_print_usage(parser);
    return -1;
}

#undef panic
#undef ensure
#undef panicf
#undef ensuref
