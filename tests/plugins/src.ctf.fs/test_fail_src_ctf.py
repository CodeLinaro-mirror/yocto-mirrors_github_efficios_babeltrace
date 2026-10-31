# SPDX-FileCopyrightText: 2019-2025 Efficios, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import bt2
import bt_tests_utils as btu


def test_ctf_2_trace_with_mip_0(ctf_traces_dir, src_ctf_comp_cls, dummy_comp_cls):
    with btu.raises_bt2_error_with_causes(
        "CTF 2 traces are not supported with MIP version 0", first_only=True
    ):
        btu.convert(
            bt2.ComponentSpec(
                src_ctf_comp_cls,
                params={"inputs": [str(ctf_traces_dir / "2/succeed/succeed1")]},
            ),
            btu.SinkComponentSpec(dummy_comp_cls),
            mip_version=0,
        )
