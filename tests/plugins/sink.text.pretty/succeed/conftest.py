# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import pathlib
import tempfile
from typing import Dict, ClassVar, FrozenSet

import bt2
import pytest
import moultipart
import bt_tests_utils as btu
import bt_tests_mctf_utils as btu_mctf


class _PrettyMctfItem(btu_mctf.MCtfTestItem):
    _allowed_non_ctf_parts: ClassVar[FrozenSet[str]] = frozenset({"@params", "@output"})
    _required_non_ctf_parts: ClassVar[FrozenSet[str]] = frozenset({"@output"})

    @staticmethod
    def _src_ctf_comp_cls() -> bt2._SourceComponentClassConst:
        return btu.plugin_by_name(
            "ctf"
        ).source_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
            "fs"
        ]

    @staticmethod
    def _pretty_comp_cls() -> bt2._SinkComponentClassConst:
        return btu.plugin_by_name(
            "text"
        ).sink_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
            "pretty"
        ]

    def _check(
        self,
        trace_dir: pathlib.Path,
        non_ctf_parts: Dict[str, moultipart.Part],
    ) -> None:
        with tempfile.TemporaryDirectory() as tmp_dir:
            out_path = pathlib.Path(tmp_dir) / "output.txt"
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
                raise btu_mctf.MCtfMismatchError(
                    self.mctf_path,
                    "Expected `sink.text.pretty` output",
                    expected_output,
                    "Actual `sink.text.pretty` output",
                    actual_output,
                )


# pytest hook.
def pytest_collect_file(file_path: pathlib.Path, parent: pytest.Collector):
    return btu_mctf.collect_mctf_file(
        file_path, parent, _PrettyMctfItem, log_label="succeed"
    )
