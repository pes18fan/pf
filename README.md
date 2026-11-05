# pf

Single-header libraries for C.

## Building

The libraries are developed as separate C and header files, but are joined
together at the end, which is the "build step". To run this build step simply
execute

```bash
./x build
```

## Testing

The test suite uses [greatest.h](https://github.com/silentbicycle/greatest).

To run the tests, execute

```bash
./x test --rebuild
```

The `--rebuild` flag can be omitted if you already pre-built the headers.

## Libraries

- **pf_str**: Simple strings and string views.
- **pf_ap**: Argument parsing library.
