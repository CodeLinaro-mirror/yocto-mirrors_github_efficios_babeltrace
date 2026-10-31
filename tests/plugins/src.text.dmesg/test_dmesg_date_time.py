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
        # Greatest time of which the number of nanoseconds fits a
        # signed 64-bit integer
        pytest.param(
            "2262-04-11 23:47:16.854",
            9_223_372_036_854_000_000,
            id="max-64-bit-ns",
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
    with btu.raises_bt2_error_with_causes(
        "Cannot convert date/time to seconds from the Unix epoch"
    ):
        list(_msg_iter(dmesg_comp_cls, tmp_path, ts))


# Tests that a `src.text.dmesg` component fails with a
# `[YYYY-MM-DD HH:MM:SS.MSEC]` timestamp of which the number of
# nanoseconds from the Unix epoch doesn't fit a signed 64-bit integer.
@pytest.mark.needs_64_bit_time_t
@pytest.mark.parametrize(
    "ts",
    [
        pytest.param("2262-04-11 23:47:16.855", id="one-msec-after-max"),
        pytest.param("2262-04-11 23:47:17.000", id="one-sec-after-max"),
        # Doesn't even fit an unsigned 64-bit integer
        pytest.param("2600-01-01 00:00:00.000", id="2600"),
    ],
)
def test_date_time_after_max(
    dmesg_comp_cls: bt2._SourceComponentClassConst,
    tmp_path: pathlib.Path,
    ts: str,
):
    with btu.raises_bt2_error_with_causes("Date/time is too far in the future"):
        list(_msg_iter(dmesg_comp_cls, tmp_path, ts))


# Tests the time of an event which a `src.text.dmesg` component creates
# from a `[SEC.USEC]` timestamp.
@pytest.mark.parametrize(
    ["ts", "expected_ns"],
    [
        pytest.param("0.000000", 0, id="zero"),
        pytest.param("0.000001", 1_000, id="one-usec"),
        pytest.param("12.345678", 12_345_678_000, id="sec-usec"),
        # Greatest time of which the number of nanoseconds fits a
        # signed 64-bit integer
        pytest.param(
            "9223372036.854775", 9_223_372_036_854_775_000, id="max-64-bit-ns"
        ),
    ],
)
def test_sec_usec(
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


# Tests that a `src.text.dmesg` component fails with a `[SEC.USEC]`
# timestamp of which the number of nanoseconds doesn't fit a signed
# 64-bit integer.
@pytest.mark.parametrize(
    "ts",
    [
        pytest.param("9223372036.854776", id="one-usec-after-max"),
        pytest.param("9223372037.000000", id="one-sec-after-max"),
        # Seconds × 10⁹ + microseconds × 10³ doesn't fit an unsigned
        # 64-bit integer
        pytest.param("18446744073.709552", id="sum-overflow"),
        # Seconds × 10⁹ doesn't fit an unsigned 64-bit integer
        pytest.param("18446744074.000000", id="sec-overflow"),
        # Microseconds × 10³ doesn't fit an unsigned 64-bit integer
        pytest.param("0.18446744073709552", id="usec-overflow"),
    ],
)
def test_sec_usec_after_max(
    dmesg_comp_cls: bt2._SourceComponentClassConst,
    tmp_path: pathlib.Path,
    ts: str,
):
    with btu.raises_bt2_error_with_causes("Timestamp is too large"):
        list(_msg_iter(dmesg_comp_cls, tmp_path, ts))
