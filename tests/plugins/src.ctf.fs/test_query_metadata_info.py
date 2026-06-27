# SPDX-FileCopyrightText: 2019-2025 Efficios, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import tempfile

import bt2
import pytest
import bt_tests_utils as btu


@pytest.fixture(scope="module")
def ctf_fs_comp_cls():
    return btu.plugin_by_name("ctf").source_component_classes["fs"]


def test_non_map_params(ctf_fs_comp_cls):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.QueryExecutor(ctf_fs_comp_cls, "metadata-info").query()

    assert "top-level is not a map value" in exc_info.value[0].message


@pytest.mark.parametrize("trace_name", ["succeed1", "lf-metadata", "crlf-metadata"])
def test_query_metadata_info(ctf_fs_comp_cls, ctf_traces_dir, trace_name):
    result = bt2.QueryExecutor(
        ctf_fs_comp_cls,
        "metadata-info",
        {"path": str(ctf_traces_dir / "1/succeed" / trace_name)},
    ).query()

    assert not result["is-packetized"]

    expect_file = btu.this_src_dir(__file__) / f"metadata-info-{trace_name}.expect"

    assert (
        str(result["text"]).replace("\r\n", "\n").strip()
        == expect_file.read_text(encoding="utf-8").strip()
    )


def test_non_existent_trace_dir(ctf_fs_comp_cls):
    with tempfile.TemporaryDirectory() as empty_dir:
        with pytest.raises(bt2._Error) as exc_info:
            bt2.QueryExecutor(
                ctf_fs_comp_cls, "metadata-info", {"path": empty_dir}
            ).query()

        assert "No such file or directory" in exc_info.value[0].message


def test_empty_metadata(ctf_fs_comp_cls, ctf_traces_dir):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.QueryExecutor(
            ctf_fs_comp_cls,
            "metadata-info",
            {"path": str(ctf_traces_dir / "1/fail/empty-metadata")},
        ).query()

    assert "Metadata stream is empty" in exc_info.value[0].message
