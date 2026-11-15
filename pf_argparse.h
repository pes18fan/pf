/*
    pf_argparse.h: CLI argument parsing library

USAGE
    Define this in your source:

    #define PF_ARGPARSE_IMPLEMENTATION
    #define "pf_argparse.h"

    This library depends on `pf.h`, so that file must be placed alongside it.

HOW TO USE
    See usage.c in the root of the repository for a simple example.

SHORT NAMES
    For convenience of use, the library exposes shorter declarations not
    qualified with the "pf_" prefix. If these conflict with the names in your
    code, define this flag before including the header:

    #define PF_ARGPARSE_NO_SHORT_NAMES
*/

#ifndef PF_ARGPARSE_H
#define PF_ARGPARSE_H

#ifndef PF_ARGPARSE_NO_SHORT_NAMES
#define ARG_MAX_FLAGS PF_ARG_MAX_FLAGS
#define ARG_MAX_SUBPARSERS PF_ARG_MAX_SUBPARSERS
#define argparser_arg_int pf_argparser_arg_int
#define argparser_arg_str pf_argparser_arg_str
#define argparser_arg_boolean pf_argparser_arg_boolean
#define argparser_arg_none pf_argparser_arg_none
#define argparser_arg_kind_t pf_argparser_arg_kind_t
#define argparser_arg_t pf_argparser_arg_t
#define argparser_flag_t pf_argparser_flag_t
#define argparser_t pf_argparser_t
#define argparser_make pf_argparser_make
#define argparser_destroy pf_argparser_destroy
#define argparser_flag_add pf_argparser_flag_add
#define argparser_print_usage pf_argparser_print_usage
#define argparse pf_argparse
#endif

#include "pf.h"

#define PF_ARG_MAX_FLAGS 16
#define PF_ARG_MAX_SUBPARSERS 16

typedef enum {
    pf_argparser_arg_int,      // The `int` type.
    pf_argparser_arg_str,      // A `const char *`.
    pf_argparser_arg_boolean,  // A `_Bool`/`bool`, or any integer type.
    pf_argparser_arg_none,     // No argument.
} pf_argparser_arg_kind_t;

// An argument taken by a flag or subcommand.
typedef struct {
    // Value of the argument, it points to a specified storage location by
    // the user.
    // Avoid accessing this data directly.
    void *_value;

    // Description of what the argument is. Can be left NULL to not provide
    // info about it.
    const char *description;

    // The type of _arg.
    // A point about 'boolean': it is similar to 'none' in how it is parsed in
    // the sense that it doesn't actually read any command line arg for its
    // value. The presence of the argument is enough for the value to be true.
    pf_argparser_arg_kind_t kind;
} pf_argparser_arg_t;

typedef struct {
    // Name of the flag.
    const char *name;

    // Short form of the flag. Can be NULL, if the flag does not have one.
    const char *short_name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Argument taken by the flag.
    pf_argparser_arg_t arg;
} pf_argparser_flag_t;

// Parser for a command. Has a set of its own subparsers.
// and flags.
typedef struct pf_argparser_t {
    // Name of the command.
    const char *name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Subparsers to parse any subcommands.
    struct pf_argparser_t *subparsers[PF_ARG_MAX_SUBPARSERS];
    int subparser_count;

    // Flags.
    pf_argparser_flag_t *flags[PF_ARG_MAX_FLAGS];
    int flag_count;

    // Argument taken by the subcommand.
    pf_argparser_arg_t arg;
} pf_argparser_t;

// Create an argument parser as the subparser of `parent`, with a subcommand
// name `name`, description `description`, argument kind `kind`, and argument
// description `arg_description`.
// If the parser is the root parser (has no parents), `parent` must be set
// to `NULL`.
// The argument the command takes is provided via `kind`, alongside the storage
// region for that value provided in `arg_ptr`.
// If the command takes no argument (`kind` == 'pf_argparser_arg_none'),
// `arg_ptr` can be set to `NULL`. If the provided parser spec has any issues
// (e.g. attempt to add a duplicate subcommand, attempt to add a subparser for a
// parser that takes an argument, addition of too many subparsers), the function
// prints the error to stderr and exits the program with status 1. `arg_ptr`
// will be one of these respective values in the case the subcommand is never
// provided (and thus no argument ever parsed):
//      - 'i': 0xdeadbeef
//      - 's': ""
//      - 'b': false
// `arg_description` can be set to NULL if it is not desired or applicable.
pf_argparser_t *pf_argparser_make(pf_argparser_t *parent, const char *name,
                                  const char *description,
                                  pf_argparser_arg_kind_t kind, void *arg_ptr,
                                  const char *arg_description);

// Deallocate an argument parser.
// Also deallocates all of its subparsers and flags; hence you should avoid
// running it for every single subparser; just destroy the root.
void pf_argparser_destroy(pf_argparser_t *parent);

// Add a flag to `parser`, with a `name` and optional `short_name` and
// `description`. The latter two can be left NULL if undesired. The type of the
// argument the flag takes is provided via `kind`, alongside the storage region
// for that value provided in `arg_ptr`.
// If the flag takes no argument (`kind` == 'pf_argparser_arg_none'), `arg_ptr`
// can be set to `NULL`. `arg_description` is the description for the argument
// shown in usage messages, it can be set to NULL if not desired or applicable.
// If the provided flag spec has any issues (e.g. invalid flag name, attempt to
// add a duplicate flag, addition of too many flags, invalid argument kind), the
// function prints the error to stderr and exits the program with status 1.
// `arg_ptr` will be one of these respective values in the case the flag is
// never provided (and thus no argument ever parsed):
//      - 'i': 0xdeadbeef
//      - 's': ""
//      - 'b': false
// `arg_description` can be set to NULL if it is not desired or applicable.
void pf_argparser_flag_add(pf_argparser_t *parser, const char *name,
                           const char *short_name, const char *description,
                           pf_argparser_arg_kind_t kind, void *arg_ptr,
                           const char *arg_description);

// Print the usage message for `parser`.
void pf_argparser_print_usage(pf_argparser_t *parser);

// Parse arguments based on the spec in `parser`.
// Returns the number of arguments parsed on success (a non-negative integer),
// otherwise returns -1.
// If it encounters a problem with the setup of the arguments (for example an
// invalid argument kind, or invalid `argv`), it prints the issue to stderr and
// exits the program with status 1.
int pf_argparse(pf_argparser_t *parser, int argc, const char *argv[]);

#endif

#ifndef PF_ARGPARSE_IMPLEMENTATION
#ifndef _PF_ARGPARSE_IMPLEMENTED
#define _PF_ARGPARSE_IMPLEMENTED

/// ----------------
/// CLI argument parsing
/// ----------------

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline const char *pf_argument_kind_str(pf_argparser_arg_kind_t k)
{
    switch (k) {
    case pf_argparser_arg_none:
        return "none";
    case pf_argparser_arg_int:
        return "int";
    case pf_argparser_arg_boolean:
        return "boolean";
    case pf_argparser_arg_str:
        return "string";
    }

    pf_panicf("invalid argument kind str %d", k);
}

pf_argparser_t *pf_argparser_make(pf_argparser_t *parent, const char *name,
                                  const char *description,
                                  pf_argparser_arg_kind_t kind, void *arg_ptr,
                                  const char *arg_description)
{
    pf_assert(name != NULL, "parser_make: parser name cannot be NULL");

    if (parent != NULL) {
        // Ensure we're not adding too many subparsers
        pf_assertf(parent->subparser_count < PF_ARG_MAX_SUBPARSERS,
                   "parser_make: failed to add subparser %s to parent %s, "
                   "parent has too many subparsers; only upto %d supported",
                   name, parent->name, PF_ARG_MAX_SUBPARSERS);

        // A parser cannot have a subparser if it takes an argument
        pf_assertf(
            parent->arg.kind == pf_argparser_arg_none,
            "parser_make: cannot add subparser %s to parser %s because the "
            "latter takes a %s argument",
            name, parent->name, pf_argument_kind_str(parent->arg.kind));

        // Ensure no duplicates
        for (int i = 0; i < parent->subparser_count; i++) {
            pf_assertf(
                strcmp(name, parent->subparsers[i]->name) != 0,
                "parser_make: cannot add subcommand %s twice to parent %s",
                name, parent->name);
        }
    }

    pf_argparser_t *p = malloc(sizeof(pf_argparser_t));
    if (!p) {
        pf_fatal(strerror(errno));
    }

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case pf_argparser_arg_int:
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case pf_argparser_arg_str:
        *((const char **) arg_ptr) = "";
        break;
    case pf_argparser_arg_boolean:
        *((bool *) arg_ptr) = false;
        break;
    case pf_argparser_arg_none:
        break;  // nothing to do
    default:
        pf_panicf("invalid argument kind %d", kind);
    }

    *p = (pf_argparser_t) {
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
    pf_argparser_flag_add(p, "--help", "-h", "Display help message",
                          pf_argparser_arg_none, NULL, NULL);

    if (parent != NULL) {
        parent->subparsers[parent->subparser_count] = p;
        parent->subparser_count++;
    }

    return p;
}

void pf_argparser_destroy(pf_argparser_t *parent)
{
    for (int i = 0; i < parent->subparser_count; i++) {
        pf_argparser_destroy(parent->subparsers[i]);
    }

    for (int i = 0; i < parent->flag_count; i++) {
        free(parent->flags[i]);
    }

    free(parent);
}

void pf_argparser_flag_add(pf_argparser_t *parser, const char *name,
                           const char *short_name, const char *description,
                           pf_argparser_arg_kind_t kind, void *arg_ptr,
                           const char *arg_description)
{
    pf_assert(name != NULL, "flag_add: long name of flag cannot be NULL");

    // Ensure not too many flags
    pf_assertf(parser->flag_count < PF_ARG_MAX_FLAGS,
               "flag_add: failed to add flag %s to parser %s, "
               "parser has too many flags; only upto %d supported",
               name, parser->name, PF_ARG_MAX_FLAGS);

    // Flag must begin with --
    pf_assertf(strncmp(name, "--", 2) == 0,
               "flag_add: invalid flag name %s, it must begin with `--`", name);

    if (short_name != NULL) {
        // Short flag must begin with -
        pf_assertf(
            strncmp(short_name, "-", 1) == 0,
            "flag_add: invalid short flag name %s, it must begin with `-`",
            name);

        // Short flag must be exactly two chars long
        pf_assertf(strlen(short_name) == 2,
                   "flag_add: invalid short flag name %s, it must be "
                   "exactly two characters long",
                   short_name);
    }

    // Ensure no duplicates, and that the --help or -h flags are not duplicated.
    // The latter is the same as the duplicate problem, but special cased to
    // provide a clearer error message.
    for (int i = 0; i < parser->flag_count; i++) {
        pf_assertf(strcmp(name, parser->flags[i]->name) != 0,
                   "flag_add: cannot add flag %s twice", name);

        if (short_name != NULL && parser->flags[i]->short_name != NULL) {
            pf_assertf(strcmp(short_name, parser->flags[i]->short_name) != 0,
                       "flag_add: cannot add flag with short name %s twice",
                       short_name);
        }
    }

    pf_argparser_flag_t *f = malloc(sizeof(pf_argparser_flag_t));
    if (!f) {
        pf_fatal(strerror(errno));
    }

    // Set some sentinel values for the cases where no argument was read
    switch (kind) {
    case pf_argparser_arg_int:
        *((int *) arg_ptr) = 0xdeadbeef;
        break;
    case pf_argparser_arg_str:
        *((const char **) arg_ptr) = "";
        break;
    case pf_argparser_arg_boolean:
        *((bool *) arg_ptr) = false;
        break;
    case pf_argparser_arg_none:
        break;  // nothing to do
    default:
        pf_panicf("invalid argument kind %d", kind);
    }

    *f = (pf_argparser_flag_t) {
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
void pf_argparser_print_usage(pf_argparser_t *parser)
{
    printf("%s", parser->name);
    if (parser->description != NULL) {
        printf(": %s", parser->description);
    }
    printf("\n\n");

    printf("Usage:\n");
    printf("    ");
    if (parser->arg.kind == pf_argparser_arg_none) {
        printf("%s", parser->name);

        if (parser->subparser_count > 0) {
            printf(" <subcommand>");
        }

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }
    } else {
        printf("%s <%s>", parser->name, pf_argument_kind_str(parser->arg.kind));

        // No subparser_count check here, as a parser that takes an argument
        // is NOT allowed to have any subcommands

        if (parser->flag_count > 0) {
            printf(" <flags>");
        }

        if (parser->arg.description != NULL) {
            printf("\n\n");
            printf("Argument of type %s: %s",
                   pf_argument_kind_str(parser->arg.kind),
                   parser->arg.description);
        }
    }
    printf("\n\n");

    printf("Subcommands:\n");
    if (parser->subparser_count == 0) {
        printf("    None\n");
    } else {
        for (int i = 0; i < parser->subparser_count; i++) {
            pf_argparser_t *subparser = parser->subparsers[i];

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
            pf_argparser_flag_t *flag = parser->flags[i];

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
static int pf_parse_argument(pf_argparser_t *parser, int argc,
                             const char *argv[])
{
    if (argc == 0 && parser->arg.kind != pf_argparser_arg_none &&
        parser->arg.kind != pf_argparser_arg_boolean) {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pf_argument_kind_str(parser->arg.kind), parser->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (parser->arg.kind) {
    case pf_argparser_arg_none:
        break;
    case pf_argparser_arg_boolean:
        *((bool *) parser->arg._value) = true;
        break;
    case pf_argparser_arg_int: {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            pf_panicf("parse: integer overflow on %ld", v);

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
    case pf_argparser_arg_str: {
        const char *arg = argv[0];
        *((const char **) parser->arg._value) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        pf_panicf("parse: invalid argument kind %d", parser->arg.kind);
    }

    return initial_argc - argc;
}

// Attempt to parse an argument of a flag. It is considered an error if no valid
// argument is found.
// Returns the number of arguments parsed on success, and -1 on error.
static int pf_parse_argument_flag(pf_argparser_flag_t *flag, int argc,
                                  const char *argv[])
{
    if (argc == 0 && flag->arg.kind != pf_argparser_arg_none &&
        flag->arg.kind != pf_argparser_arg_boolean) {
        fprintf(stderr, "parse: %s argument for %s not provided\n",
                pf_argument_kind_str(flag->arg.kind), flag->name);
        return -1;
    }

    int initial_argc = argc;

    // Parse the argument
    // For now this just means consuming the thing
    switch (flag->arg.kind) {
    case pf_argparser_arg_none:
        break;
    case pf_argparser_arg_boolean:
        *((bool *) flag->arg._value) = true;
        break;
    case pf_argparser_arg_int: {
        const char *arg = argv[0];
        char *endptr;
        errno = 0;
        long v = strtol(arg, &endptr, 10);

        // Overflow/underflow, should just panic here
        if (errno == ERANGE || v > INT_MAX || v < INT_MIN)
            pf_panicf("parse: integer overflow on %ld", v);

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
    case pf_argparser_arg_str: {
        const char *arg = argv[0];
        *((const char **) flag->arg._value) = arg;

        argv++;
        argc--;
        break;
    }
    default:
        pf_panicf("parse: invalid argument kind %d", flag->arg.kind);
    }

    return initial_argc - argc;
}

// Parse flags if found. If no args are remaining it doesn't do anything.
// Returns a whole number (0 or higher) on success, which is the number of flags
// parsed. Returns -1 on error.
static int pf_parse_flags(pf_argparser_t *parser, int argc, const char *argv[])
{
    if (argc == 0)
        return 0;

    int initial_argc = argc;

    // If we find a flag, start parsing them
    while (argc > 0 &&
           (strncmp(argv[0], "--", 2) == 0 || strncmp(argv[0], "-", 1) == 0)) {
        const char *flag_name = argv[0];
        pf_argparser_flag_t *received_flag = NULL;

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
            pf_argparser_print_usage(parser);
            return -1;
        }

        // If we received the help flag just print usage and exit
        if (strcmp(received_flag->name, "--help") == 0 ||
            (received_flag->short_name != NULL &&
             strcmp(received_flag->short_name, "-h") == 0)) {
            pf_argparser_print_usage(parser);
            exit(0);
        }

        argv++;
        argc--;

        // Try to parse argument
        int skipped = pf_parse_argument_flag(received_flag, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    return initial_argc - argc;
}

int pf_argparse(pf_argparser_t *parser, int argc, const char *argv[])
{
    int initial_argc = argc;
    int skipped;

    pf_assert(argc >= 1,
              "parse: invalid arguments provided, need at least 1 arg\n");

    // Consume the first arg (program / subcommand name)
    argv++;
    argc--;

    // Try to parse flags
    skipped = pf_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // If the parser has any subparsers, parse them recursively
    if (argc > 0 && parser->subparser_count > 0) {
        const char *subcommand_name = argv[0];
        pf_argparser_t *received_subparser = NULL;
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

        skipped = pf_argparse(received_subparser, argc, argv);
        if (skipped == -1)
            return skipped;

        argv += skipped;
        argc -= skipped;
    }

    // Try to parse flags... again
    skipped = pf_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    // Try to parse argument
    skipped = pf_parse_argument(parser, argc, argv);
    if (skipped == -1)
        goto die;

    argv += skipped;
    argc -= skipped;

    // Parse any remaining flags...
    skipped = pf_parse_flags(parser, argc, argv);
    if (skipped == -1)
        return skipped;

    argv += skipped;
    argc -= skipped;

    return initial_argc - argc;

die:
    pf_argparser_print_usage(parser);
    return -1;
}

#endif  // _PF_ARGPARSE_IMPLEMENTED
#endif  // PF_ARGPARSE_IMPLEMENTATION
