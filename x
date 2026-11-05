#!/usr/bin/env python
import shutil
import textwrap
import subprocess
from typing import Sequence
from dataclasses import dataclass
from pathlib import Path
import argparse
import platform
import os
import urllib.request

root = Path(__file__).resolve().parent
greatest_src = root / "greatest.h"
test_src = root / "test.c"
src = root / "src"


@dataclass(frozen=True)
class Lib:
    source: Path
    header: Path
    impl_macro: str


libs = [
    Lib(src / "pf_str.c", src / "pf_str.h", "PF_STR_IMPLEMENTATION"),
    Lib(src / "pf_ap.c", src / "pf_ap.h", "PF_AP_IMPLEMENTATION"),
]


def build(_args: argparse.Namespace):
    print("building headers...")
    for lib in libs:
        source = lib.source
        header = lib.header
        impl_macro = lib.impl_macro
        output = os.path.basename(lib.header)

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
        print(f"\tbuilt header {output}")
    print("all headers built.")


def test(args: argparse.Namespace):
    if args.rebuild:
        build(args)

    if not os.path.exists(greatest_src):
        url = "https://raw.githubusercontent.com/silentbicycle/greatest/refs/heads/release/greatest.h"
        urllib.request.urlretrieve(url, root / "greatest.h")

    test_bin = root / "test.bin"
    print("testing...")
    try:
        subprocess.run(
            [
                "cc",
                str(test_src),
                "-o",
                str(test_bin),
                "-Wall",
                "-Wextra",
                "-fsanitize=address",
            ],
            check=True,
            capture_output=True,
        )
        result = subprocess.run([test_bin], check=True, capture_output=True)
        print(result.stdout.decode("utf-8"))
        os.remove(test_bin)
    except subprocess.CalledProcessError as e:
        print(
            textwrap.dedent(f"""
            command failed: {" ".join(e.cmd)}:
                {e.stderr.decode("utf-8")}
            testing failed with exit status: {e.returncode}
            """)
        )
    else:
        print("tests passed!")


def main() -> int | None:
    parser = argparse.ArgumentParser(prog="build")
    subparsers = parser.add_subparsers(dest="command", required=True)

    build_p = subparsers.add_parser("build", help="Build the headers")
    build_p.set_defaults(func=build)

    test_p = subparsers.add_parser("test", help="Run tests")
    test_p.add_argument(
        "-r",
        "--rebuild",
        help="Rebuild the headers before testing",
        action="store_true",
    )
    test_p.set_defaults(func=test)

    args = parser.parse_args()
    args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
