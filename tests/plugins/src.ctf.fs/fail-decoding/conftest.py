# SPDX-FileCopyrightText: 2025 Efficios, Inc.
# SPDX-License-Identifier: GPL-2.0-only

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import pathlib
from typing import Dict, ClassVar, FrozenSet

import bt2
import pytest
import moultipart
import bt_tests_utils as btu
import bt_tests_mctf_utils as btu_mctf


class _FailMctfItem(btu_mctf.MCtfTestItem):
    _allowed_non_ctf_parts: ClassVar[FrozenSet[str]] = frozenset({"@error"})
    _required_non_ctf_parts: ClassVar[FrozenSet[str]] = frozenset({"@error"})

    def _check(
        self,
        trace_dir: pathlib.Path,
        non_ctf_parts: Dict[str, moultipart.Part],
    ) -> None:
        src_ctf_comp_cls: bt2._SourceComponentClassConst = btu.plugin_by_name(
            "ctf"
        ).source_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
            "fs"
        ]
        dummy_comp_cls: bt2._SinkComponentClassConst = btu.plugin_by_name(
            "utils"
        ).sink_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
            "dummy"
        ]

        expected_error = non_ctf_parts["@error"].content.strip()
        expected_label = "Expected the graph to raise an error containing"

        try:
            btu.convert(
                bt2.ComponentSpec(
                    src_ctf_comp_cls, params={"inputs": [str(trace_dir)]}
                ),
                btu.SinkComponentSpec(dummy_comp_cls),
            )
        except bt2._Error as exc:
            if expected_error not in exc[0].message:
                raise btu_mctf.MCtfMismatchError(
                    self.mctf_path,
                    expected_label,
                    expected_error,
                    "Actual error message",
                    exc[0].message,
                )
        else:
            raise btu_mctf.MCtfNoErrorError(
                self.mctf_path, expected_label, expected_error
            )


# pytest hook.
def pytest_collect_file(file_path: pathlib.Path, parent: pytest.Collector):
    return btu_mctf.collect_mctf_file(
        file_path, parent, _FailMctfItem, log_label="fail"
    )
