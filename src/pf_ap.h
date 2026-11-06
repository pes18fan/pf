#ifndef PF_AP_H
#define PF_AP_H

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#error C11 support required
#endif

#define MAX_FLAGS 16
#define MAX_SUBPARSERS 16

#ifndef PF_AP_NO_SHORT_NAMES
#define arg_int pfa_arg_int
#define arg_str pfa_arg_str
#define arg_boolean pfa_arg_boolean
#define arg_none pfa_arg_none
#define arg_kind pfa_arg_kind

#define arg pfa_arg

#define flag pfa_flag

#define argparser pfa_argparser
#define argparser_make pfa_argparser_make
#define argparser_destroy pfa_argparser_destroy

#define flag_add pfa_flag_add

#define print_usage pfa_print_usage

#define argparse pfa_argparse
#endif

typedef enum {
    pfa_arg_int,      // The `int` type.
    pfa_arg_str,      // A `const char *`.
    pfa_arg_boolean,  // A `_Bool`/`bool`, or any integer type.
    pfa_arg_none,     // No argument.
} pfa_arg_kind;

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
    pfa_arg_kind kind;
} pfa_arg;

typedef struct {
    // Name of the flag.
    const char *name;

    // Short form of the flag. Can be NULL, if the flag does not have one.
    const char *short_name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Argument taken by the flag.
    pfa_arg arg;
} pfa_flag;

// Parser for a command. Has a set of its own subparsers.
// and flags.
typedef struct pfa_argparser {
    // Name of the command.
    const char *name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Subparsers to parse any subcommands.
    struct pfa_argparser *subparsers[MAX_SUBPARSERS];
    int subparser_count;

    // Flags.
    pfa_flag *flags[MAX_FLAGS];
    int flag_count;

    // Argument taken by the subcommand.
    pfa_arg arg;
} pfa_argparser;

// Create an argument parser as the subparser of `parent`, with a subcommand
// name `name`, description `description`, argument kind `kind`, and argument
// description `arg_description`.
// If the parser is the root parser (has no parents), `parent` must be set
// to `NULL`.
// The argument the command takes is provided via `kind`, alongside the storage
// region for that value provided in `arg_ptr`.
// If the command takes no argument (`kind` == 'pfa_arg_none'), `arg_ptr` can
// be set to `NULL`. If the provided parser spec has any issues (e.g. attempt to
// add a duplicate subcommand, attempt to add a subparser for a parser that
// takes an argument, addition of too many subparsers), the function prints the
// error to stderr and exits the program with status 1. `arg_ptr` will be one of
// these respective values in the case the subcommand is never provided (and
// thus no argument ever parsed):
//      - 'i': 0xdeadbeef
//      - 's': ""
//      - 'b': false
// `arg_description` can be set to NULL if it is not desired or applicable.
pfa_argparser *pfa_argparser_make(pfa_argparser *parent, const char *name,
                                  const char *description, pfa_arg_kind kind,
                                  void *arg_ptr, const char *arg_description);

// Deallocate an argument parser.
// Also deallocates all of its subparsers and flags; hence you should avoid
// running it for every single subparser; just destroy the root.
void pfa_argparser_destroy(pfa_argparser *parent);

// Add a flag to `parser`, with a `name` and optional `short_name` and
// `description`. The latter two can be left NULL if undesired. The type of the
// argument the flag takes is provided via `kind`, alongside the storage region
// for that value provided in `arg_ptr`.
// If the flag takes no argument (`kind` == 'pfa_arg_none'), `arg_ptr` can be
// set to `NULL`.
// `arg_description` is the description for the argument shown in
// usage messages, it can be set to NULL if not desired or applicable.
// If the provided flag spec has any issues (e.g. invalid flag name, attempt to
// add a duplicate flag, addition of too many flags, invalid argument kind), the
// function prints the error to stderr and exits the program with status 1.
// `arg_ptr` will be one of these respective values in the case the flag is
// never provided (and thus no argument ever parsed):
//      - 'i': 0xdeadbeef
//      - 's': ""
//      - 'b': false
// `arg_description` can be set to NULL if it is not desired or applicable.
void pfa_flag_add(pfa_argparser *parser, const char *name,
                  const char *short_name, const char *description,
                  pfa_arg_kind kind, void *arg_ptr,
                  const char *arg_description);

// Print the usage message for `parser`.
void pfa_print_usage(pfa_argparser *parser);

// Parse arguments based on the spec in `parser`.
// Returns the number of arguments parsed on success (a non-negative integer),
// otherwise returns -1.
// If it encounters a problem with the setup of the arguments (for example an
// invalid argument kind, or invalid `argv`), it prints the issue to stderr and
// exits the program with status 1.
int pfa_argparse(pfa_argparser *parser, int argc, const char *argv[]);

#endif
