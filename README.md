# pf

Single-header libraries intended as a standard library replacement for C.

Still VERY incomplete!

## What does it provide?

- pf.h, provides some basic important machinery
    - Panics and assertions.
    - Strings and string views.
    - Type generic lists.
- pf_allocator.h
    - An interface for creating custom allocators.
    - Provides three implementations of this interface:
        - An implementation for normal C `malloc`, `realloc` and `free`,
        - An implementation wrapping the above and zeroing out new allocations.
            This is the default.
        - An arena allocator implementation that allocates on a provided backing
            buffer.
- pf_argparse.h
    - A CLI argument parser.
