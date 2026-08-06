# SPDX-FileCopyrightText: 2025 Philippe Proulx <pproulx@efficios.com>
# SPDX-License-Identifier: MIT

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import os
import typing
import logging
import pathlib
import textwrap
import subprocess
from typing import Any, Dict, List, Union, Optional

import bt2
import bt_tests_utils as btu

_logger = logging.getLogger(__name__)


# Anything cli_params_from_obj() below can format.
CliParamsObj = typing.Union[
    bt2._ValueConst,
    bool,
    int,
    float,
    str,
    List["CliParamsObj"],
    Dict[str, "CliParamsObj"],
    None,
]


# Formats `obj` as a `babeltrace2` CLI `--params` value string.
#
# This function doesn't escape special characters in strings.
def cli_params_from_obj(obj: CliParamsObj, is_root: bool = True) -> str:
    if obj is None:
        return "null"

    if isinstance(obj, bool) or isinstance(obj, bt2._BoolValueConst):
        return "yes" if obj else "no"

    if (isinstance(obj, int) and obj >= 0) or isinstance(
        obj, bt2._UnsignedIntegerValueConst
    ):
        # Prefix non-negative integers with `+` so the CLI parameter
        # parser builds an unsigned integer value (the bare `N` form
        # produces a signed integer).
        return f"+{obj}"

    if isinstance(obj, int) or isinstance(obj, bt2._SignedIntegerValueConst):
        return str(obj)

    if isinstance(obj, float) or isinstance(obj, bt2._RealValueConst):
        return f"{float(obj):.7f}"

    if isinstance(obj, str) or isinstance(obj, bt2._StringValueConst):
        return f'"{obj}"'

    if isinstance(obj, list) or isinstance(obj, bt2._ArrayValueConst):
        return f"[{', '.join((cli_params_from_obj(item, False) for item in obj))}]"

    if isinstance(obj, dict) or isinstance(obj, bt2._MapValueConst):
        # Since this is used by tests that expect an exact string, sort the
        # entries by key to make the output stable.
        items = ", ".join(
            f"{k}={cli_params_from_obj(v, False)}" for k, v in sorted(obj.items())
        )

        return items if is_root else f"{{{items}}}"

    raise TypeError("Unexpected type", type(obj))


# `babeltrace2` CLI parameters to be passed to `run_cli()`.
class CliParams:
    def __init__(self, params: Dict[str, Any]):
        assert len(params) > 0
        self._params = params

    @property
    def params(self):
        return self._params


# Runs the `babeltrace2` CLI compiled within `build_root_dir`
# (the build directory containing the `tests` directory) with the
# arguments `args` considering the plugin paths `plugin_paths`.
#
# Each item of `args` may be a string or a `CliParams` instance. A
# `CliParams` instance is converted to `--params` followed by the
# equivalent parameters value string.
#
# `check`, `timeout`, and `extra_env` are passed to btu.run().
def run_cli(
    build_root_dir: pathlib.Path,
    args: List[Union[str, CliParams]],
    plugin_paths: Optional[List[Any]] = None,
    check: bool = False,
    timeout: Optional[float] = None,
    extra_env: Optional[Dict[str, str]] = None,
) -> "subprocess.CompletedProcess[str]":
    cli_args: List[str] = []

    if plugin_paths is not None:
        cli_args += [
            "--plugin-path",
            (";" if btu.os_type() == btu.OsType.MINGW else ":").join(
                str(p) for p in plugin_paths
            ),
        ]

    for arg in args:
        if isinstance(arg, CliParams):
            cli_args += ["-p", cli_params_from_obj(arg.params)]
        else:
            cli_args.append(arg)

    return btu.run(
        build_root_dir,
        build_root_dir / "src/cli/babeltrace2",
        cli_args,
        check=check,
        timeout=timeout,
        extra_env=extra_env,
    )


# Runs a graph with `sink.text.details` and compares the output to the
# expectation `expect` (a string or the path of an expectation file
# to open).
#
# `cli_args` is a list of arguments to pass to the CLI before the
# `sink.text.details` component. See the `args` parameter of run_cli().
#
# `details_params` is a dictionary of initialization parameters to pass
# to the `sink.text.details` component.
#
# `expect` is the expected output, either:
#
# • A `pathlib.Path` object (expectation file path).
# • A string (dedented before comparison).
#
# The `sink.text.details` component receives its `color` initialization
# parameter set to `never`, therefore the expectation string must not
# contain terminal color codes.
#
# `plugin_paths` is an optional list of plugin paths to pass to the CLI.
#
# This function asserts that the CLI output matches the expectation
# (both stripped of leading/trailing whitespace).
#
# Returns the `subprocess.CompletedProcess` result.
def run_cli_sink_text_details_test(
    build_root_dir: pathlib.Path,
    cli_args: List[Union[str, CliParams]],
    details_params: Dict[str, Any],
    expect: Union[pathlib.Path, str],
    plugin_paths: Optional[List[pathlib.Path]] = None,
) -> "subprocess.CompletedProcess[str]":
    full_cli_args = list(cli_args) + [
        "-c",
        "sink.text.details",
        CliParams({"color": "never"}),
    ]

    if details_params:
        full_cli_args.append(CliParams(details_params))

    result = run_cli(
        build_root_dir,
        full_cli_args,
        plugin_paths=plugin_paths,
        check=True,
    )

    if isinstance(expect, pathlib.Path):
        expected = expect.read_text(encoding="utf-8").strip()
    else:
        expected = textwrap.dedent(expect).strip()

    output = result.stdout.strip()

    if output != expected:
        old_expected = expected

        if (
            isinstance(expect, pathlib.Path)
            and os.environ.get("BT_TESTS_WRITE_EXPECTED") == "1"
        ):
            expect.write_text(f"{output}\n", encoding="utf-8")

        assert output == old_expected

    return result
