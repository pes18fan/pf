#include "pf_ap.h"

#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Simple wrapper for panic with formatted errors
#define panic(message, ...)                      \
    do {                                         \
        fprintf(stderr, "panic: ");              \
        fprintf(stderr, (message), __VA_ARGS__); \
        exit(1);                                 \
    } while (0)

// Assertion with formatted errors
#define ensure(expr, message, ...)         \
    do {                                   \
        if (!(expr)) {                     \
            panic((message), __VA_ARGS__); \
        }                                  \
    } while (0)

static inline const char *pf_ap_argument_kind_str(char k)
{
    switch (k) {
    case 'n':
        return "none";
    case 'i':
        return "int";
    case 'b':
        return "boolean";
    case 's':
        return "string";
    }

    panic("invalid argument kind str %c", k);
}

pf_ap *pf_ap_make(pf_ap *parent, const char *name, const char *description,
                  char kind, void *arg_ptr)
{
    if (parent != NULL) {
        // Ensure we're not adding too many subparsers
        ensure(parent->subparser_count < MAX_SUBPARSERS,
               "parser_make: failed to add subparser %s to parent %s, "
               "parent has too many subparsers; only upto %d supported",
               name, parent->name, MAX_SUBPARSERS);

        // A parser cannot have a subparser if it takes an argument
        ensure(parent->arg_kind == 'n',
               "parser_make: cannot add subparser %s to parser %s because the "
               "latter takes a %s argument",
               name, parent->name, pf_ap_argument_kind_str(parent->arg_kind));

        // Ensure no duplicates
        for (int i = 0; i < parent->subparser_count; i++) {
            ensure(strcmp(name, parent->subparsers[i]->name) != 0,
                   "parser_make: cannot add subcommand %s twice to parent %s",
                   name, parent->name);
        }
    }

    // Ensure the arg kind is not invalid
    ensure(kind == 'i' || kind == 'n' || kind == 's' || kind == 'b',
           "parser_make: invalid arg_kind %c", kind);

    struct pf_ap *p = malloc(sizeof(struct pf_ap));
    ensure(p != NULL, "parser_make: fatal error: %s", strerror(errno));

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case 'i':
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case 's':
        *((char **) arg_ptr) = "\0";
        break;
    case 'b':
        *((bool *) arg_ptr) = false;
        break;
    case 'n':
        break;  // nothing to do
    default:
        panic("%s", "unreachable");
    }

    *p = (struct pf_ap){
        .name = name != NULL ? name : "parser",
        .description = description,
        .subparser_count = 0,
        .flag_count = 0,
        .arg_kind = kind,
        ._arg = arg_ptr,
    };

    memset(p->subparsers, 0, sizeof p->subparsers);
    memset(p->flags, 0, sizeof p->flags);

    // Add --help flag
    pf_ap_flag_add(p, "--help", "-h", "Display help message", 'n', NULL);

    if (parent != NULL) {
        parent->subparsers[parent->subparser_count] = p;
        parent->subparser_count++;
    }

    return p;
}

void pf_ap_destroy(pf_ap *parent)
{
    for (int i = 0; i < parent->subparser_count; i++) {
        pf_ap_destroy(parent->subparsers[i]);
    }

    for (int i = 0; i < parent->flag_count; i++) {
        free(parent->flags[i]);
    }

    free(parent);
}

void pf_ap_flag_add(pf_ap *parser, const char *name, const char *short_name,
                    const char *description, char kind, void *arg_ptr)
{
    // Ensure not too many flags
    ensure(parser->flag_count < MAX_FLAGS,
           "flag_add: failed to add flag %s to parser %s, "
           "parser has too many flags; only upto %d supported",
           name, parser->name, MAX_FLAGS);

    // Flag must begin with --
    ensure(strncmp(name, "--", 2) == 0,
           "flag_add: invalid flag name %s, it must begin with `--`", name);

    if (short_name != NULL) {
        // Short flag must begin with -
        ensure(strncmp(short_name, "-", 1) == 0,
               "flag_add: invalid short flag name %s, it must begin with `-`",
               name);

        // Short flag must be exactly two chars long
        ensure(strlen(short_name) == 2,
               "flag_add: invalid short flag name %s, it must be "
               "exactly two characters long",
               short_name);
    }

    // Ensure no duplicates, and that the --help or -h flags are not duplicated.
    // The latter is the same as the duplicate problem, but special cased to
    // provide a clearer error message.
    for (int i = 0; i < parser->flag_count; i++) {
        ensure(strcmp(name, parser->flags[i]->name) != 0,
               "flag_add: cannot add flag %s twice", name);

        if (short_name != NULL) {
            ensure(strcmp(short_name, parser->flags[i]->short_name) != 0,
                   "flag_add: cannot add flag with short name %s twice",
                   short_name);
        }
    }

    // Ensure the arg kind is not invalid
    ensure(kind == 'i' || kind == 'n' || kind == 's' || kind == 'b',
           "flag_add: invalid arg_kind %c", kind);

    pf_ap_flag *f = malloc(sizeof(pf_ap_flag));
    ensure(f != NULL, "flag_add: fatal error: %s", strerror(errno));

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case 'i':
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case 's':
        *((const char **) arg_ptr) = "";
        break;
    case 'b':
        *((bool *) arg_ptr) = false;
        break;
    case 'n':
        break;  // nothing to do
    default:
        panic("%s", "unreachable");
    }

    *f = (pf_ap_flag){
        .name = name,
        .description = description,
        .short_name = short_name,
        .arg_kind = kind,
        ._arg = arg_ptr,
    };

    parser->flags[parser->flag_count] = f;
    parser->flag_count++;
}

// Print a parser usage message.
// This is the output of the program when the --help or -h flag is invoked, and
// is also shown on argument parsing errors.
void pf_ap_print_usage(pf_ap *parser)
{
    printf("%s", parser->name);
    if (parser->description != NULL) {
        printf(": %s", parser->description);
    }
    printf("\n\n");

    printf("Usage:\n");
    printf("    ");
    if (parser->arg_kind == 'n') {
        printf("%s", parser->name);

        if (parser->subparser_count > 0) {
            printf(" <subcommand>");
        }

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }
    } else {
        printf("%s <%s>", parser->name,
               pf_ap_argument_kind_str(parser->arg_kind));

        // No subparser_count check here, as a parser that takes an argument
        // is NOT allowed to have any subcommands

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }
    }
    printf("\n\n");

    printf("Subcommands:\n");
    if (parser->subparser_count == 0) {
        printf("    None\n");
    } else {
        for (int i = 0; i < parser->subparser_count; i++) {
            struct pf_ap *subparser = parser->subparsers[i];

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
            pf_ap_flag *flag = parser->flags[i];

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
static int pf_ap_parse_argument(pf_ap *parser, int argc, const char *argv[])
{
    if (argc == 0 && parser->arg_kind != 'n' && parser->arg_kind != 'b') {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pf_ap_argument_kind_str(parser->arg_kind), parser->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (parser->arg_kind) {
    case 'n':
        break;
    case 'b':
        *((bool *) parser->_arg) = true;
        break;
    case 'i': {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            panic("parse: integer overflow on %ld", v);

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

        *((int *) parser->_arg) = (int) v;

        argv++;
        argc--;
        break;
    }
    case 's': {
        const char *arg = argv[0];
        *((const char **) parser->_arg) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        panic("parse: invalid argument kind %d", parser->arg_kind);
    }

    return initial_argc - argc;
}

// Attempt to parse an argument. It is considered an error if no valid argument
// is found.
// Returns the number of arguments parsed on success, and -1 on error.
static int pf_ap_parse_argument_flag(pf_ap_flag *flag, int argc,
                                     const char *argv[])
{
    if (argc == 0 && flag->arg_kind != 'n' && flag->arg_kind != 'b') {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pf_ap_argument_kind_str(flag->arg_kind), flag->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (flag->arg_kind) {
    case 'n':
        break;
    case 'b':
        *((bool *) flag->_arg) = true;
        break;
    case 'i': {
        const char *arg = argv[0];
        char *endptr;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            panic("parse: integer overflow on %ld", v);

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

        *((int *) flag->_arg) = (int) v;

        argv++;
        argc--;
        break;
    }
    case 's': {
        const char *arg = argv[0];
        *((const char **) flag->_arg) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        panic("parse: invalid argument kind %d", flag->arg_kind);
    }

    return initial_argc - argc;
}

// Parse flags if found. If no args are remaining it doesn't do anything.
// Returns a whole number (0 or higher) on success, which is the number of flags
// parsed. Returns -1 on error.
static int pf_ap_parse_flags(pf_ap *parser, int argc, const char *argv[])
{
    if (argc == 0)
        return 0;

    int initial_argc = argc;

    // If we find a flag, start parsing them
    while (argc > 0 &&
           (strncmp(argv[0], "--", 2) == 0 || strncmp(argv[0], "-", 1) == 0)) {
        const char *flag_name = argv[0];
        pf_ap_flag *received_flag = NULL;

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
            pf_ap_print_usage(parser);
            return -1;
        }

        // If we received the help flag just print usage and exit
        if (strcmp(received_flag->short_name, "--help") == 0 ||
            (received_flag->short_name &&
             strcmp(received_flag->short_name, "-h") == 0)) {
            pf_ap_print_usage(parser);
            exit(0);
        }

        argv++;
        argc--;

        // Try to parse argument
        int skipped = pf_ap_parse_argument_flag(received_flag, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    return initial_argc - argc;
}

int pf_ap_parse(pf_ap *parser, int argc, const char *argv[])
{
    int initial_argc = argc;
    int skipped;

    ensure(argc >= 1,
           "parse: invalid arguments provided, need at least 1 arg\n", 0);

    // Consume the first arg (program / subcommand name)
    argv++;
    argc--;

    // Try to parse flags
    skipped = pf_ap_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // If the parser has any subparsers, parse them recursively
    if (argc > 0 && parser->subparser_count > 0) {
        const char *subcommand_name = argv[0];
        pf_ap *received_subparser = NULL;
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

        skipped = pf_ap_parse(received_subparser, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    // Try to parse flags... again
    skipped = pf_ap_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // Try to parse argument
    skipped = pf_ap_parse_argument(parser, argc, argv);
    if (skipped == -1)
        goto die;

    argv += skipped;
    argc -= skipped;

    // Parse any remaining flags...
    skipped = pf_ap_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    return initial_argc - argc;

die:
    pf_ap_print_usage(parser);
    return -1;
}

#undef panic
#undef ensure
