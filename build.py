#!/usr/bin/env python
from os.path import basename
from dataclasses import dataclass
from pathlib import Path
import argparse
import platform

root = Path(__file__).resolve().parent
src = root / "src"


@dataclass(frozen=True)
class Lib:
    source: Path
    header: Path
    impl_macro: str


libs = [Lib(src / "pf_str.c", src / "pf_str.h", "PF_STR_IMPLEMENTATION")]


def main() -> int | None:
    for lib in libs:
        source = lib.source
        header = lib.header
        impl_macro = lib.impl_macro
        output = basename(lib.header)

        lines = []
        with open(header, "r") as f:
            lines = f.readlines()

        for line in reversed(lines):
            lines = lines[:-1]
            if line.startswith("#endif"):
                break

        source_lines = []
        with open(source, "r") as f:
            source_lines = f.readlines()

        lines.append(f"#ifdef {impl_macro}\n")
        lines += source_lines
        lines.append(f"#endif // {impl_macro}\n")
        lines.append("#endif\n")

        with open(output, "w") as f:
            f.writelines(lines)


if __name__ == "__main__":
    raise SystemExit(main())
