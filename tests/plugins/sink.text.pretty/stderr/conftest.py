# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import re
import typing
import logging
import pathlib
import tempfile
from typing import Any, Dict, List, Tuple, Optional

import mctf
import pytest
import bt_tests_utils as btu
import bt_tests_cli_utils as btu_cli


class _StderrMctfMismatchException(Exception):
    def __init__(
        self, mctf_path: pathlib.Path, expected_pattern: str, actual_stderr: str
    ) -> None:
        super().__init__()
        self._mctf_path = mctf_path
        self._expected_pattern = expected_pattern
        self._actual_stderr = actual_stderr

    def format_output(self) -> str:
        return "\n".join(
            [
                f"📄 Test file: `{self._mctf_path}`",
                "",
                "✅ Expected `sink.text.pretty` standard error regex:",
                self._expected_pattern,
                "",
                "❌ Actual `sink.text.pretty` standard error:",
                self._actual_stderr,
            ]
        )


class _StderrMctfItem(pytest.Item):
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
                    "@stderr-re",
                }, f"Unexpected non-CTF part `{header_info}` in `{self._mctf_path}`"

            assert (
                "@stderr-re" in non_ctf_parts
            ), f"Missing `@stderr` part in `{self._mctf_path}`"

            pretty_params: Dict[str, Any] = {"color": "never"}

            if "@params" in non_ctf_parts:
                pretty_params.update(
                    btu.params_from_part(non_ctf_parts["@params"].content)
                )

            actual_stderr = btu_cli.run_cli(
                typing.cast(pathlib.Path, getattr(self.config, "build_root_dir")),
                [
                    "-c",
                    "src.ctf.fs",
                    btu_cli.CliParams(
                        {
                            "inputs": [str(trace_dir)],
                            "trace-name": "the-trace",
                        }
                    ),
                    "-c",
                    "sink.text.pretty",
                    "--log-level=W",
                    btu_cli.CliParams(pretty_params),
                ],
                check=True,
            ).stderr.strip()

            expected_pattern = non_ctf_parts["@stderr-re"].content.strip()

            if not re.fullmatch(expected_pattern, actual_stderr):
                raise _StderrMctfMismatchException(
                    self._mctf_path, expected_pattern, actual_stderr
                )

    def reportinfo(self) -> Tuple[Any, None, str]:
        return self.path, None, self.name

    def repr_failure(
        self,
        excinfo: "pytest.ExceptionInfo[BaseException]",
        style: Any = None,
    ) -> Any:
        if isinstance(excinfo.value, _StderrMctfMismatchException):
            return excinfo.value.format_output()

        return super().repr_failure(excinfo, style)


class _StderrMctfFile(pytest.File):
    def collect(self) -> List[_StderrMctfItem]:
        logging.getLogger(__name__).info(f"Adding stderr test from `{self.path}`")

        return [
            _StderrMctfItem.from_parent(  # pyright: ignore[reportUnknownMemberType]
                name="test",
                parent=self,
                mctf_path=self.path,
            )
        ]


# pytest hook.
def pytest_collect_file(
    file_path: pathlib.Path, parent: pytest.Collector
) -> Optional[_StderrMctfFile]:
    if file_path.suffix != ".mctf":
        return

    return _StderrMctfFile.from_parent(  # pyright: ignore[reportUnknownMemberType]
        parent=parent, path=file_path
    )
