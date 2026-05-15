# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import typing
import logging
import pathlib
import tempfile
from typing import Any, List, Tuple, Optional

import mctf
import pytest
import bt_tests_utils as btu
import bt_tests_cli_utils as btu_cli


class _CounterMctfOutputMismatchException(Exception):
    def __init__(
        self, mctf_path: pathlib.Path, expected_output: str, actual_output: str
    ) -> None:
        super().__init__()
        self._mctf_path = mctf_path
        self._expected_output = expected_output
        self._actual_output = actual_output

    def format_output(self) -> str:
        return "\n".join(
            [
                f"📄 Test file: `{self._mctf_path}`",
                "",
                "✅ Expected `sink.utils.counter` output:",
                self._expected_output,
                "",
                "❌ Actual `sink.utils.counter` output:",
                self._actual_output,
            ]
        )


class _CounterMctfItem(pytest.Item):
    def __init__(self, *, mctf_path: pathlib.Path, **kwargs: Any) -> None:
        super().__init__(**kwargs)  # pyright: ignore[reportUnknownMemberType]
        self._mctf_path = mctf_path

    def runtest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            trace_dir = pathlib.Path(tmp_dir) / self._mctf_path.stem
            non_ctf_parts = mctf.generate(str(self._mctf_path), str(trace_dir), False)

            for header_info in non_ctf_parts:
                assert header_info in {
                    "@params",
                    "@output",
                }, f"Unexpected non-CTF part `{header_info}` in `{self._mctf_path}`"

            assert (
                "@output" in non_ctf_parts
            ), f"Missing `@output` part in `{self._mctf_path}`"

            cli_args: List[typing.Union[str, btu_cli.CliParams]] = [
                "-c",
                "src.ctf.fs",
                btu_cli.CliParams({"inputs": [str(trace_dir)]}),
                "-c",
                "sink.utils.counter",
            ]

            if "@params" in non_ctf_parts:
                counter_params = btu.params_from_part(non_ctf_parts["@params"].content)

                if counter_params:
                    cli_args.append(btu_cli.CliParams(counter_params))

            actual_output = btu_cli.run_cli(
                typing.cast(pathlib.Path, getattr(self.config, "build_root_dir")),
                cli_args,
                check=True,
            ).stdout.strip()

            expected_output = non_ctf_parts["@output"].content.strip()

            if actual_output != expected_output:
                raise _CounterMctfOutputMismatchException(
                    self._mctf_path, expected_output, actual_output
                )

    def reportinfo(self) -> Tuple[Any, None, str]:
        return self.path, None, self.name

    def repr_failure(
        self,
        excinfo: "pytest.ExceptionInfo[BaseException]",
        style: Any = None,
    ) -> Any:
        if isinstance(excinfo.value, _CounterMctfOutputMismatchException):
            return excinfo.value.format_output()

        return super().repr_failure(excinfo, style)


class _CounterMctfFile(pytest.File):
    def collect(self) -> List[_CounterMctfItem]:
        logging.getLogger(__name__).info(f"Adding succeed test from `{self.path}`")

        return [
            _CounterMctfItem.from_parent(  # pyright: ignore[reportUnknownMemberType]
                name="test",
                parent=self,
                mctf_path=self.path,
            )
        ]


# pytest hook.
def pytest_collect_file(
    file_path: pathlib.Path, parent: pytest.Collector
) -> Optional[_CounterMctfFile]:
    if file_path.suffix != ".mctf":
        return

    return _CounterMctfFile.from_parent(  # pyright: ignore[reportUnknownMemberType]
        parent=parent, path=file_path
    )
