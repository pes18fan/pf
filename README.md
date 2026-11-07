# pf

Single-header libraries for C.

Still very incomplete!

## Building

The libraries are developed as separate C and header files, but are joined
together at the end, which is the "build step". To run this build step simply
execute

```bash
./x build
```

## Libraries

- **pf_str**: Simple strings and string views.
- **pf_ap**: Argument parsing library.
