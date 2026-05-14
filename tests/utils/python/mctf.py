# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2023-2026 EfficiOS Inc.
#
# pyright: strict

import os
import sys
import typing
import argparse
from typing import Any, Dict, List, Union

import normand
import moultipart


class ErrorCause:
    def __init__(self, what: str, line_no: int, col_no: int):
        self._what = what
        self._line_no = line_no
        self._col_no = col_no

    @property
    def what(self):
        return self._what

    @property
    def line_no(self):
        return self._line_no

    @property
    def col_no(self):
        return self._col_no


class Error(RuntimeError):
    def __init__(self, causes: List[ErrorCause]):
        self._causes = causes

    @property
    def causes(self):
        return self._causes


def _write_file(
    name: str, base_dir: str, content: Union[str, bytearray], verbose: bool
):
    path = os.path.join(base_dir, name)

    if verbose:
        print(f"Writing `{os.path.normpath(path)}`.")

    os.makedirs(os.path.normpath(os.path.dirname(path)), exist_ok=True)

    if isinstance(content, str):
        with open(path, "w", encoding="utf-8") as f:
            f.write(content)
    else:
        with open(path, "wb") as f:
            f.write(content)


def _normand_parse(
    part: moultipart.Part, init_vars: normand.VariablesT, init_labels: normand.LabelsT
):
    try:
        return normand.parse(
            part.content, init_variables=init_vars, init_labels=init_labels
        )
    except normand.ParseError as e:
        raise Error(
            [
                ErrorCause(
                    msg.text,
                    msg.text_location.line_no + part.first_content_line_no - 1,
                    msg.text_location.col_no,
                )
                for msg in e.messages
            ]
        ) from e


def _reformat_ctf_2_metadata(content: str):
    import json

    json_seq_fragments: List[str] = []

    for fragment in json.loads(content):
        json_seq_fragments.append(f"\x1e{json.dumps(fragment, indent=2)}")

    return "\n".join(json_seq_fragments)


def _reformat_metadata(content: str):
    content = f"{content.strip()}\n"

    if content.startswith("["):
        # CTF 2: JSON array to JSON text sequence
        return _reformat_ctf_2_metadata(content)

    return content


def _generate_from_part(
    part: moultipart.Part,
    base_dir: str,
    verbose: bool,
    normand_vars: normand.VariablesT,
    normand_labels: normand.LabelsT,
):
    content = part.content

    if part.header_info == "metadata":
        content = _reformat_metadata(content)
    else:
        res = _normand_parse(part, normand_vars, normand_labels)
        content = res.data
        normand_vars = res.variables
        normand_labels = res.labels

    _write_file(part.header_info, base_dir, content, verbose)
    return normand_vars, normand_labels


def generate(
    input_path: str, base_dir: str, verbose: bool
) -> Dict[str, moultipart.Part]:
    with open(input_path, encoding="utf-8") as input_file:
        variables: normand.VariablesT = {}
        labels: normand.LabelsT = {}
        non_ctf_parts: Dict[str, moultipart.Part] = {}

        for part in moultipart.parse(input_file):
            if part.header_info.startswith("@"):
                assert part.header_info not in non_ctf_parts
                non_ctf_parts[part.header_info] = part
                continue

            variables, labels = _generate_from_part(
                part, base_dir, verbose, variables, labels
            )

        return non_ctf_parts


def _parse_cli_args():
    argparser = argparse.ArgumentParser()
    argparser.add_argument(
        "input_path", metavar="PATH", type=str, help="moultipart input file name"
    )
    argparser.add_argument(
        "--base-dir", type=str, help="base directory of generated files", default=""
    )
    argparser.add_argument(
        "--verbose", "-v", action="store_true", help="increase verbosity"
    )
    return argparser.parse_args()


def _run_cli(args: Any):
    generate(
        typing.cast(str, args.input_path),
        typing.cast(str, args.base_dir),
        typing.cast(bool, args.verbose),
    )


def _try_run_cli():
    args = _parse_cli_args()

    try:
        _run_cli(args)
    except Error as exc:
        print("Failed to process Normand part:", file=sys.stderr)

        for cause in reversed(exc.causes):
            print(
                f"  {os.path.abspath(args.input_path)}:{cause.line_no}:{cause.col_no} - {cause.what}{'.' if cause.what[-1] not in '.:;' else ''}",
                file=sys.stderr,
            )

        sys.exit(1)


if __name__ == "__main__":
    _try_run_cli()
