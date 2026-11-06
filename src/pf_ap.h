#ifndef pf_ap_h
#define pf_ap_h

#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#error C11 support required
#endif

#define MAX_FLAGS 16
#define MAX_SUBPARSERS 16

#ifndef PF_AP_NO_SHORT_NAMES
#define pf_ap_arg_int arg_int
#define pf_ap_arg_str arg_str
#define pf_ap_arg_boolean arg_boolean
#define pf_ap_arg_none arg_none
#define pf_ap_arg_kind arg_kind

#define pf_ap_arg arg

#define pf_ap_flag flag

#define pf_ap_argparser argparser
#define pf_ap_argparser_make argparser_make
#define pf_ap_argparser_destroy argparser_destroy

#define pf_ap_flag_add flag_add

#define pf_ap_print_usage print_usage

#define pf_ap_argparse argparse
#endif

typedef enum {
    pf_ap_arg_int,      // The `int` type.
    pf_ap_arg_str,      // A `const char *`.
    pf_ap_arg_boolean,  // A `_Bool`/`bool`, or any integer type.
    pf_ap_arg_none,     // No argument.
} pf_ap_arg_kind;

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
    pf_ap_arg_kind kind;
} pf_ap_arg;

typedef struct {
    // Name of the flag.
    const char *name;

    // Short form of the flag. Can be NULL, if the flag does not have one.
    const char *short_name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Argument taken by the flag.
    pf_ap_arg arg;
} pf_ap_flag;

// Parser for a command. Has a set of its own subparsers.
// and flags.
typedef struct pf_ap_argparser {
    // Name of the command.
    const char *name;

    // Description. Can be left NULL to not provide any of it.
    const char *description;

    // Subparsers to parse any subcommands.
    struct pf_ap_argparser *subparsers[MAX_SUBPARSERS];
    int subparser_count;

    // Flags.
    pf_ap_flag *flags[MAX_FLAGS];
    int flag_count;

    // Argument taken by the subcommand.
    pf_ap_arg arg;
} pf_ap_argparser;

// Create an argument parser as the subparser of `parent`, with a subcommand
// name `name`, description `description`, argument kind `kind`, and argument
// description `arg_description`.
// If the parser is the root parser (has no parents), `parent` must be set
// to `NULL`.
// The argument the command takes is provided via `kind`, alongside the storage
// region for that value provided in `arg_ptr`.
// If the command takes no argument (`kind` == 'pf_ap_arg_none'), `arg_ptr` can
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
pf_ap_argparser *pf_ap_argparser_make(pf_ap_argparser *parent, const char *name,
                                      const char *description,
                                      pf_ap_arg_kind kind, void *arg_ptr,
                                      const char *arg_description);

// Deallocate an argument parser.
// Also deallocates all of its subparsers and flags; hence you should avoid
// running it for every single subparser; just destroy the root.
void pf_ap_argparser_destroy(pf_ap_argparser *parent);

// Add a flag to `parser`, with a `name` and optional `short_name` and
// `description`. The latter two can be left NULL if undesired. The type of the
// argument the flag takes is provided via `kind`, alongside the storage region
// for that value provided in `arg_ptr`.
// If the flag takes no argument (`kind` == 'pf_ap_arg_none'), `arg_ptr` can be
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
void pf_ap_flag_add(pf_ap_argparser *parser, const char *name,
                    const char *short_name, const char *description,
                    pf_ap_arg_kind kind, void *arg_ptr,
                    const char *arg_description);

// Print the usage message for `parser`.
void pf_ap_print_usage(pf_ap_argparser *parser);

// Parse arguments based on the spec in `parser`.
// Returns the number of arguments parsed on success (a non-negative integer),
// otherwise returns -1.
// If it encounters a problem with the setup of the arguments (for example an
// invalid argument kind, or invalid `argv`), it prints the issue to stderr and
// exits the program with status 1.
int pf_ap_argparse(pf_ap_argparser *parser, int argc, const char *argv[]);

#endif
