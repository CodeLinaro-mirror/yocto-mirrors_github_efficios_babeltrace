# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import pathlib

import bt2
import pytest
import bt_tests_utils as btu


# Returns a message iterator on a `src.text.dmesg` component reading
# a file, within `tmp_path`, which contains a single line having the
# `[ts]` timestamp.
def _msg_iter(
    dmesg_comp_cls: bt2._SourceComponentClassConst, tmp_path: pathlib.Path, ts: str
) -> bt2.TraceCollectionMessageIterator:
    path = tmp_path / "dmesg.txt"
    path.write_text(f"[{ts}] meow\n")
    return bt2.TraceCollectionMessageIterator(
        [bt2.ComponentSpec(dmesg_comp_cls, params={"path": str(path)})],
        plugin_set=btu.plugin_set(),
    )


# Tests the time of an event which a `src.text.dmesg` component creates
# from a `[YYYY-MM-DD HH:MM:SS.MSEC]` timestamp (UTC).
@pytest.mark.parametrize(
    ["ts", "expected_ns"],
    [
        pytest.param("1970-01-01 00:00:01.000", 1_000_000_000, id="epoch"),
        pytest.param(
            "2026-07-15 12:00:00.250", 1_784_116_800_250_000_000, id="before-2038"
        ),
        # Greatest time which fits a signed 32-bit integer (seconds)
        pytest.param(
            "2038-01-19 03:14:07.000", 2_147_483_647_000_000_000, id="max-32-bit"
        ),
        pytest.param(
            "2038-01-19 03:14:08.000",
            2_147_483_648_000_000_000,
            id="after-max-32-bit",
            marks=pytest.mark.needs_64_bit_time_t,
        ),
        pytest.param(
            "2040-02-29 00:00:00.000",
            2_214_086_400_000_000_000,
            id="leap-day-2040",
            marks=pytest.mark.needs_64_bit_time_t,
        ),
        pytest.param(
            "2100-03-01 00:00:00.000",
            4_107_542_400_000_000_000,
            id="no-leap-day-2100",
            marks=pytest.mark.needs_64_bit_time_t,
        ),
    ],
)
def test_date_time(
    dmesg_comp_cls: bt2._SourceComponentClassConst,
    tmp_path: pathlib.Path,
    ts: str,
    expected_ns: int,
):
    msgs = _msg_iter(dmesg_comp_cls, tmp_path, ts)

    assert [
        msg.default_clock_snapshot.ns_from_origin
        for msg in msgs
        if isinstance(msg, bt2._EventMessageConst)
    ] == [expected_ns]


# Tests that a `src.text.dmesg` component fails with a
# `[YYYY-MM-DD HH:MM:SS.MSEC]` timestamp before the Unix epoch, which a
# clock snapshot can't represent.
@pytest.mark.parametrize(
    "ts",
    [
        pytest.param("1969-12-31 23:59:59.000", id="one-sec-before-epoch"),
        pytest.param("1900-01-01 00:00:00.000", id="1900"),
    ],
)
def test_date_time_before_epoch(
    dmesg_comp_cls: bt2._SourceComponentClassConst,
    tmp_path: pathlib.Path,
    ts: str,
):
    msgs = _msg_iter(dmesg_comp_cls, tmp_path, ts)

    with pytest.raises(bt2._Error) as exc_info:
        list(msgs)

    assert any(
        "Cannot convert date/time to seconds from the Unix epoch" in cause.message
        for cause in exc_info.value
    ), [cause.message for cause in exc_info.value]
