#!/usr/bin/env python
from dataclasses import dataclass
from pathlib import Path
from os.path import basename, splitext
import argparse

root = Path(__file__).resolve().parent
src = root / "src"


@dataclass(frozen=True)
class Lib:
    source: Path
    header: Path
    impl_macro: str
    one_line_description: str
    declaration_qualifier: str
    short_name_disabler_macro: str | None


libs = [
    Lib(
        source=src / "pf_str.c",
        header=src / "pf_str.h",
        impl_macro="PF_STR_IMPLEMENTATION",
        one_line_description="string and string view manipulation library",
        declaration_qualifier="pfs",
        short_name_disabler_macro="PF_STR_NO_SHORT_NAMES",
    ),
    Lib(
        source=src / "pf_ap.c",
        header=src / "pf_ap.h",
        impl_macro="PF_AP_IMPLEMENTATION",
        one_line_description="argument parsing library inspired by Python's argparse",
        declaration_qualifier="pfa",
        short_name_disabler_macro="PF_AP_NO_SHORT_NAMES",
    ),
]


def build(_args: argparse.Namespace):
    print("building headers...")
    for lib in libs:
        source = lib.source
        header = lib.header
        impl_macro = lib.impl_macro
        one_line_description = lib.one_line_description
        qualifier = lib.declaration_qualifier
        short_name_disabler_macro = lib.short_name_disabler_macro
        output = basename(lib.header)

        lines = [
            f"/* {basename(header)} - {one_line_description}\n",
            "\n",
            "   To use this library, do this in exactly *one* C file:\n",
            "\n",
            f"   #define {impl_macro}\n",
            f'   #include "{basename(header)}"\n',
        ]

        if short_name_disabler_macro is not None:
            lines += [
                "\n",
                f"   For convenience of use, the library exposes shorter declarations not\n",
                f'   qualified with the "{qualifier}_" prefix. If these conflict with names in your\n',
                "   code, define this flag before including the header: \n",
                "\n",
                f"   #define {short_name_disabler_macro}\n",
            ]
        lines.append("*/\n")

        with open(header, "r") as f:
            lines += f.readlines()

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
        print(f"\tbuilt header {output}")
    print("all headers built.")


def main() -> int | None:
    parser = argparse.ArgumentParser(prog="build")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_p = subparsers.add_parser("build", help="Build the headers")
    build_p.set_defaults(func=build)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
