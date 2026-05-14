# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import logging
import pathlib
import tempfile
from typing import Any, Dict, List, Tuple, Optional

import bt2
import mctf
import pytest
import bt_tests_utils as btu


class _PrettyMctfOutputMismatchException(Exception):
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
                "✅ Expected `sink.text.pretty` output:",
                self._expected_output,
                "",
                "❌ Actual `sink.text.pretty` output:",
                self._actual_output,
            ]
        )


class _PrettyMctfItem(pytest.Item):
    def __init__(self, *, mctf_path: pathlib.Path, **kwargs: Any) -> None:
        super().__init__(**kwargs)  # pyright: ignore[reportUnknownMemberType]
        self._mctf_path = mctf_path

    @staticmethod
    def _src_ctf_comp_cls() -> bt2._SourceComponentClassConst:
        return bt2.find_plugin("ctf").source_component_classes["fs"]

    @staticmethod
    def _pretty_comp_cls() -> bt2._SinkComponentClassConst:
        return bt2.find_plugin("text").sink_component_classes["pretty"]

    def runtest(self) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            tmp_path = pathlib.Path(tmp_dir)
            trace_dir = tmp_path / self._mctf_path.stem
            non_ctf_parts = mctf.generate(str(self._mctf_path), str(trace_dir), False)

            for header_info in non_ctf_parts:
                assert header_info in {
                    "@params",
                    "@output",
                }, f"Unexpected non-CTF part `{header_info}` in `{self._mctf_path}`"

            assert (
                "@output" in non_ctf_parts
            ), f"Missing `@output` part in `{self._mctf_path}`"

            out_path = tmp_path / "output.txt"
            params: Dict[str, bt2._ComponentParams] = {
                "color": "never",
                "path": str(out_path),
            }

            if "@params" in non_ctf_parts:
                params.update(btu.params_from_part(non_ctf_parts["@params"].content))

            btu.convert(
                bt2.ComponentSpec(
                    self._src_ctf_comp_cls(), params={"inputs": [str(trace_dir)]}
                ),
                btu.SinkComponentSpec(self._pretty_comp_cls(), params),
            )

            expected_output = non_ctf_parts["@output"].content.strip()
            actual_output = out_path.read_text(encoding="utf-8").strip()

            if actual_output != expected_output:
                raise _PrettyMctfOutputMismatchException(
                    self._mctf_path, expected_output, actual_output
                )

    def reportinfo(self) -> Tuple[Any, None, str]:
        return self.path, None, self.name

    def repr_failure(
        self,
        excinfo: "pytest.ExceptionInfo[BaseException]",
        style: Any = None,
    ) -> Any:
        if isinstance(excinfo.value, _PrettyMctfOutputMismatchException):
            return excinfo.value.format_output()

        return super().repr_failure(excinfo, style)


class _PrettyMctfFile(pytest.File):
    def collect(self) -> List[_PrettyMctfItem]:
        logging.getLogger(__name__).info(f"Adding succeed test from `{self.path}`")

        return [
            _PrettyMctfItem.from_parent(  # pyright: ignore[reportUnknownMemberType]
                name="test",
                parent=self,
                mctf_path=self.path,
            )
        ]


# pytest hook.
def pytest_collect_file(
    file_path: pathlib.Path, parent: pytest.Collector
) -> Optional[_PrettyMctfFile]:
    if file_path.suffix != ".mctf":
        return

    return _PrettyMctfFile.from_parent(  # pyright: ignore[reportUnknownMemberType]
        parent=parent, path=file_path
    )
