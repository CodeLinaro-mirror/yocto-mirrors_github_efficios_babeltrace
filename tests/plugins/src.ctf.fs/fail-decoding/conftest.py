# SPDX-FileCopyrightText: 2025 Efficios, Inc.
# SPDX-License-Identifier: GPL-2.0-only

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import logging
import pathlib
import tempfile
from typing import Any, List, Tuple, Optional

import bt2
import mctf
import pytest
import bt_tests_utils as btu


class _FailMctfMismatchException(Exception):
    def __init__(self, mctf_path: pathlib.Path, expected: str, actual: str) -> None:
        super().__init__()
        self._mctf_path = mctf_path
        self._expected = expected
        self._actual = actual

    def format_output(self) -> str:
        return "\n".join(
            [
                f"📄 Test file: `{self._mctf_path}`",
                "",
                "✅ Expected the graph to raise an error containing:",
                self._expected,
                "",
                "❌ Actual error message:",
                self._actual,
            ]
        )


class _FailMctfNoErrorException(Exception):
    def __init__(self, mctf_path: pathlib.Path, expected: str) -> None:
        super().__init__()
        self._mctf_path = mctf_path
        self._expected = expected

    def format_output(self) -> str:
        return "\n".join(
            [
                f"📄 Test file: `{self._mctf_path}`",
                "",
                "✅ Expected the graph to raise an error containing:",
                self._expected,
                "",
                "❌ Actual: the graph completed without raising an error.",
            ]
        )


class _FailMctfItem(pytest.Item):
    def __init__(self, *, mctf_path: pathlib.Path, **kwargs: Any) -> None:
        super().__init__(**kwargs)  # pyright: ignore[reportUnknownMemberType]
        self._mctf_path = mctf_path

    def runtest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            trace_dir = pathlib.Path(tmp_dir) / self._mctf_path.stem
            non_ctf_parts = mctf.generate(str(self._mctf_path), str(trace_dir), False)

            src_ctf_comp_cls: bt2._SourceComponentClassConst = bt2.find_plugin(
                "ctf"
            ).source_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
                "fs"
            ]
            dummy_comp_cls: bt2._SinkComponentClassConst = bt2.find_plugin(
                "utils"
            ).sink_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
                "dummy"
            ]

            assert len(non_ctf_parts) == 1
            assert "@error" in non_ctf_parts
            expected_error = non_ctf_parts["@error"].content.strip()

            try:
                btu.convert(
                    bt2.ComponentSpec(
                        src_ctf_comp_cls, params={"inputs": [str(trace_dir)]}
                    ),
                    btu.SinkComponentSpec(dummy_comp_cls),
                )
            except bt2._Error as exc:
                if expected_error not in exc[0].message:
                    raise _FailMctfMismatchException(
                        self._mctf_path, expected_error, exc[0].message
                    )
            else:
                raise _FailMctfNoErrorException(self._mctf_path, expected_error)

    def reportinfo(self) -> Tuple[Any, None, str]:
        return self.path, None, self.name

    def repr_failure(
        self,
        excinfo: "pytest.ExceptionInfo[BaseException]",
        style: Any = None,
    ) -> Any:
        if isinstance(
            excinfo.value, (_FailMctfMismatchException, _FailMctfNoErrorException)
        ):
            return excinfo.value.format_output()

        return super().repr_failure(excinfo, style)


class _FailMctfFile(pytest.File):
    def collect(self) -> List[_FailMctfItem]:
        logging.getLogger(__name__).info(f"Adding fail test from `{self.path}`")

        return [
            _FailMctfItem.from_parent(  # pyright: ignore[reportUnknownMemberType]
                name="test",
                parent=self,
                mctf_path=self.path,
            )
        ]


# pytest hook.
def pytest_collect_file(
    file_path: pathlib.Path, parent: pytest.Collector
) -> Optional[_FailMctfFile]:
    if file_path.suffix != ".mctf" or not file_path.name.startswith(
        ("ctf-1.8-", "ctf-2-")
    ):
        return

    return _FailMctfFile.from_parent(  # pyright: ignore[reportUnknownMemberType]
        parent=parent, path=file_path
    )
