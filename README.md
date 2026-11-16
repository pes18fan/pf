# pf

Single-header libraries intended as a standard library replacement for C.

Still VERY incomplete!

## What does it provide?

- pf.h, provides some basic important machinery
    - Panics and assertions.
    - An interface for creating custom allocators, `allocator_t`.
        - Functions that operate on allocators implementing this interface.
        - Provides three readymade implementations of this interface:
            - An implementation for normal C `malloc`, `realloc` and `free`,
            - An implementation wrapping the above and zeroing out new allocations.
                This is the default.
            - An arena allocator implementation that allocates on a provided backing
                buffer.
    - Strings (`str_t`) and string views (`strv_t`).
        - Various functions to manipulate strings.
    - Type generic lists, `list_t`.
- pf_allocator.h
- pf_argparse.h
    - A CLI argument parser.
