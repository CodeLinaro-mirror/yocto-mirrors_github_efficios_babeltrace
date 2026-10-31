# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import time
import types
import datetime
from typing import Any, Dict, List, Union, Callable, Iterator, Optional

import bt2
import pytest
import bt_tests_utils as btu


# Returns the nanoseconds from the Unix epoch of the UTC date/time
# `date_time` (`YYYY-MM-DDTHH:MM`).
def _ns_from_utc(date_time: str) -> int:
    dt = datetime.datetime.strptime(date_time, "%Y-%m-%dT%H:%M").replace(
        tzinfo=datetime.timezone.utc
    )

    return int(dt.timestamp()) * 1_000_000_000


# Returns the event name for the nanoseconds from the Unix epoch `ns`:
# the UTC date/time `YYYY-MM-DDTHH:MM`.
def _event_name(ns: int) -> str:
    return datetime.datetime.fromtimestamp(
        ns // 1_000_000_000, datetime.timezone.utc
    ).strftime("%Y-%m-%dT%H:%M")


class _MsgIter(bt2._UserMessageIterator):
    def __init__(self, config, port):
        stream = port.user_data.tc().create_stream(port.user_data.sc)
        self._msgs = [self._create_stream_beginning_message(stream)]

        for ns in port.user_data.times:
            self._msgs.append(
                self._create_event_message(
                    port.user_data.ec(_event_name(ns)), stream, ns
                )
            )

        self._msgs.append(self._create_stream_end_message(stream))
        self._at = 0
        config.can_seek_forward = True

    def _user_seek_beginning(self):
        self._at = 0

    def __next__(self):
        if self._at < len(self._msgs):
            msg = self._msgs[self._at]
            self._at += 1
            return msg

        raise StopIteration


# Source of which the single stream contains one event message for each
# time of the `times` parameter (UTC date/times, `YYYY-MM-DDTHH:MM`),
# the name of the event class being the time.
class _Src(bt2._UserSourceComponent, message_iterator_class=_MsgIter):
    def __init__(self, config, params, obj):
        tc = self._create_trace_class()
        sc = tc.create_stream_class(
            default_clock_class=self._create_clock_class(
                frequency=1_000_000_000, origin_is_unix_epoch=True
            ),
        )
        times = [_ns_from_utc(str(t)) for t in params["times"]]
        ecs = {}

        def ec(name: str):
            if name not in ecs:
                ecs[name] = sc.create_event_class(name=name)

            return ecs[name]

        self._add_output_port(
            "out", types.SimpleNamespace(tc=tc, sc=sc, ec=ec, times=times)
        )


# Returns the names of the events which a trimmer having the
# initialization parameters `trimmer_params` lets through, the source
# having an event for each UTC date/time of `times`.
def _trimmed_events(
    trimmer_comp_cls, times: List[str], trimmer_params: Dict[str, Any]
) -> List[str]:
    return [
        ev.name
        for ev in btu.tcmi_events(
            [bt2.ComponentSpec(_Src, params={"times": times})],
            [bt2.ComponentSpec(trimmer_comp_cls, params=trimmer_params)],
        )
    ]


# Sets the `TZ` environment variable of this process (local time zone)
# with the returned function, restoring it on teardown.
#
# time.tzset() makes the C library apply the new value.
#
# Skips the requesting test when time.tzset() isn't available
# (Windows).
@pytest.fixture
def set_local_tz() -> Iterator[Callable[[str], None]]:
    if not hasattr(time, "tzset"):
        pytest.skip("time.tzset() isn't available")

    with pytest.MonkeyPatch.context() as mp:

        def set_tz(tz: str):
            mp.setenv("TZ", tz)
            time.tzset()

        yield set_tz

    # `mp` restored `TZ` on exit
    time.tzset()


# One event every hour, from 10:00 to 15:00 UTC on 1 July 2024.
_HOURLY = [f"2024-07-01T{h:02}:00" for h in range(10, 16)]


# Tests the `tz` entry of the `ts-format` parameter with the date/time
# and time formats of the `begin` and `end` parameters.
@pytest.mark.parametrize(
    ["params", "expected"],
    [
        pytest.param(
            {
                "ts-format": {"tz": "utc"},
                "begin": "2024-07-01 12:00",
                "end": "2024-07-01 13:30",
            },
            _HOURLY[2:4],
            id="utc",
        ),
        pytest.param(
            {
                "ts-format": {"tz": "+00:00"},
                "begin": "2024-07-01 12:00",
                "end": "2024-07-01 13:30",
            },
            _HOURLY[2:4],
            id="offset-zero",
        ),
        pytest.param(
            {
                "ts-format": {"tz": "-04:00"},
                "begin": "2024-07-01 08:00",
                "end": "2024-07-01 09:30",
            },
            _HOURLY[2:4],
            id="offset-neg",
        ),
        pytest.param(
            {
                "ts-format": {"tz": "+05:30"},
                "begin": "2024-07-01 17:30",
                "end": "2024-07-01 19:00",
            },
            _HOURLY[2:4],
            id="offset-pos-half-hour",
        ),
        pytest.param(
            {
                "ts-format": {"tz": "+14:00"},
                "begin": "2024-07-02 02:00",
                "end": "2024-07-02 03:30",
            },
            _HOURLY[2:4],
            id="offset-next-day",
        ),
        pytest.param(
            {
                "ts-format": {"tz": "-11:00"},
                "begin": "2024-06-30 23:00",
                "end": "2024-07-01 00:30",
            },
            _HOURLY[0:2],
            id="offset-prev-day",
        ),
        pytest.param(
            {"ts-format": {"tz": "+05:30"}, "begin": "2024-07-01"},
            _HOURLY,
            id="offset-date-only",
        ),
        pytest.param(
            {"ts-format": {"tz": "+14:00"}, "begin": "2024-07-02"},
            _HOURLY,
            id="offset-date-only-next-day",
        ),
        pytest.param(
            {"ts-format": {"tz": "-04:00"}, "begin": "08:00", "end": "09:30"},
            _HOURLY[2:4],
            id="offset-time-only",
        ),
        # The first event is at 00:00 on 2 July in UTC+14:00: 02:00
        # means 2 July, 12:00 UTC.
        pytest.param(
            {"ts-format": {"tz": "+14:00"}, "begin": "02:00"},
            _HOURLY[2:],
            id="offset-time-only-next-day",
        ),
        # The first event is at 23:00 on 30 June in UTC-11:00: 01:00
        # means 30 June, 12:00 UTC, which is before the first event.
        pytest.param(
            {"ts-format": {"tz": "-11:00"}, "begin": "01:00"},
            _HOURLY,
            id="offset-time-only-prev-day",
        ),
        pytest.param(
            {"ts-format": {"tz": "-11:00"}, "end": "01:30"},
            [],
            id="offset-time-only-prev-day-end",
        ),
        pytest.param(
            {"gmt": True, "begin": "2024-07-01 12:00", "end": "2024-07-01 13:30"},
            _HOURLY[2:4],
            id="gmt",
        ),
    ],
)
def test_tz(trimmer_comp_cls, params: Dict[str, Any], expected: List[str]):
    assert _trimmed_events(trimmer_comp_cls, _HOURLY, params) == expected


# Tests the `tz` entry of the `ts-format` parameter with time
# zone identifiers.
@pytest.mark.parametrize(
    ["times", "params", "expected"],
    [
        # EDT (UTC-04:00)
        pytest.param(
            _HOURLY,
            {
                "ts-format": {"tz": "America/Toronto"},
                "begin": "2024-07-01 08:00",
                "end": "2024-07-01 09:30",
            },
            _HOURLY[2:4],
            id="toronto-summer",
            marks=btu.needs_zoneinfo("America/Toronto"),
        ),
        # EST (UTC-05:00)
        pytest.param(
            ["2024-01-15T11:00", "2024-01-15T12:00", "2024-01-15T13:00"],
            {"ts-format": {"tz": "America/Toronto"}, "begin": "2024-01-15 07:00"},
            ["2024-01-15T12:00", "2024-01-15T13:00"],
            id="toronto-winter",
            marks=btu.needs_zoneinfo("America/Toronto"),
        ),
        # Windows time zone name (only on Windows), EDT (UTC-04:00)
        pytest.param(
            _HOURLY,
            {
                "ts-format": {"tz": "Eastern Standard Time"},
                "begin": "2024-07-01 08:00",
                "end": "2024-07-01 09:30",
            },
            _HOURLY[2:4],
            id="eastern-summer-windows",
            marks=btu.needs_zoneinfo("Eastern Standard Time"),
        ),
        pytest.param(
            _HOURLY,
            {"ts-format": {"tz": "America/Toronto"}, "begin": "08:00", "end": "09:30"},
            _HOURLY[2:4],
            id="toronto-time-only",
            marks=btu.needs_zoneinfo("America/Toronto"),
        ),
        pytest.param(
            _HOURLY,
            {
                "ts-format": {"tz": "Asia/Kolkata"},
                "begin": "2024-07-01 17:30",
                "end": "2024-07-01 19:00",
            },
            _HOURLY[2:4],
            id="kolkata",
            marks=btu.needs_zoneinfo("Asia/Kolkata"),
        ),
        pytest.param(
            _HOURLY,
            {
                "ts-format": {"tz": "UTC"},
                "begin": "2024-07-01 12:00",
                "end": "2024-07-01 13:30",
            },
            _HOURLY[2:4],
            id="utc-id",
            marks=btu.needs_zoneinfo("UTC"),
        ),
        # 01:30 occurs twice on 3 November 2024 in Toronto (05:30 and
        # 06:30 UTC): the first occurrence (EDT) wins.
        pytest.param(
            [
                "2024-11-03T05:00",
                "2024-11-03T05:30",
                "2024-11-03T06:00",
                "2024-11-03T06:30",
                "2024-11-03T07:00",
            ],
            {"ts-format": {"tz": "America/Toronto"}, "begin": "2024-11-03 01:30"},
            [
                "2024-11-03T05:30",
                "2024-11-03T06:00",
                "2024-11-03T06:30",
                "2024-11-03T07:00",
            ],
            id="toronto-dst-end-ambiguous",
            marks=btu.needs_zoneinfo("America/Toronto"),
        ),
        # 02:30 doesn't exist on 10 March 2024 in Toronto: it becomes
        # 03:00 EDT (07:00 UTC).
        pytest.param(
            [
                "2024-03-10T06:30",
                "2024-03-10T07:00",
                "2024-03-10T07:30",
                "2024-03-10T08:00",
            ],
            {"ts-format": {"tz": "America/Toronto"}, "begin": "2024-03-10 02:30"},
            ["2024-03-10T07:00", "2024-03-10T07:30", "2024-03-10T08:00"],
            id="toronto-dst-start-nonexistent",
            marks=btu.needs_zoneinfo("America/Toronto"),
        ),
    ],
)
def test_tz_id(
    trimmer_comp_cls, times: List[str], params: Dict[str, Any], expected: List[str]
):
    assert _trimmed_events(trimmer_comp_cls, times, params) == expected


# Tests that the `tz` entry of the `ts-format` parameter has no effect
# on the seconds format of the `begin` and `end` parameters.
@pytest.mark.parametrize(
    "tz", ["local", "utc", "+05:30", "-11:00", "+14:00", "Asia/Kolkata"]
)
@pytest.mark.parametrize(
    ["begin", "end"],
    [
        pytest.param(
            str(_ns_from_utc("2024-07-01T12:00") // 1_000_000_000),
            str(_ns_from_utc("2024-07-01T13:00") // 1_000_000_000) + ".5",
            id="str",
        ),
        pytest.param(
            _ns_from_utc("2024-07-01T12:00") // 1_000_000_000,
            _ns_from_utc("2024-07-01T13:00") // 1_000_000_000 + 1,
            id="int",
        ),
    ],
)
def test_tz_sec(trimmer_comp_cls, tz: str, begin: Any, end: Any):
    if tz == "Asia/Kolkata" and not btu.zoneinfo_has(tz):
        pytest.skip(f"time zone database lacks {tz}")

    assert (
        _trimmed_events(
            trimmer_comp_cls,
            _HOURLY,
            {"ts-format": {"tz": tz}, "begin": begin, "end": end},
        )
        == _HOURLY[2:4]
    )


# Tests the `local` value of the `tz` entry of the `ts-format`
# parameter, which is also the default, with a local time zone
# at UTC+03:00.
@pytest.mark.parametrize(
    "params",
    [
        pytest.param(
            {
                "ts-format": {"tz": "local"},
                "begin": "2024-07-01 15:00",
                "end": "2024-07-01 16:30",
            },
            id="local-date-time",
        ),
        pytest.param(
            {"ts-format": {"tz": "local"}, "begin": "15:00", "end": "16:30"},
            id="local-time",
        ),
        pytest.param(
            {"begin": "2024-07-01 15:00", "end": "2024-07-01 16:30"},
            id="default-date-time",
        ),
        pytest.param(
            {"ts-format": {}, "begin": "2024-07-01 15:00", "end": "2024-07-01 16:30"},
            id="empty-ts-format-date-time",
        ),
        pytest.param(
            {"gmt": False, "begin": "2024-07-01 15:00", "end": "2024-07-01 16:30"},
            id="gmt-false",
        ),
    ],
)
def test_tz_local(trimmer_comp_cls, set_local_tz, params: Dict[str, Any]):
    # UTC+03:00 without DST, as a POSIX `TZ` value which doesn't need
    # the time zone database: `XYZ` is an arbitrary abbreviation and
    # `-3` is the offset, in hours _west_ of UTC (hence the
    # inverted sign).
    #
    # See
    # <https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html>.
    set_local_tz("XYZ-3")
    assert _trimmed_events(trimmer_comp_cls, _HOURLY, params) == _HOURLY[2:4]


# Error cause message of an invalid `tz` entry of the `ts-format`
# parameter, which follows the cause message of the actual reason.
_INVALID_TZ_MSG = "Invalid `tz` entry of `ts-format` parameter."


# Tests that the component rejects invalid `ts-format` parameters and
# date/times with the expected error cause message(s).
@pytest.mark.parametrize(
    ["params", "expected_msgs"],
    [
        pytest.param(
            {"ts-format": "utc"},
            "Error validating parameter `ts-format`: unexpected type: "
            "expected-type=MAP",
            id="ts-format-not-map",
        ),
        pytest.param(
            {"ts-format": {"zone": "utc"}},
            "Error validating parameter `ts-format`: unexpected key `zone`.",
            id="ts-format-unknown-key",
        ),
        pytest.param(
            {"ts-format": {"tz": 23}},
            "Error validating parameter `ts-format.tz`: unexpected type",
            id="tz-not-str",
        ),
        *[
            pytest.param(
                {"ts-format": {"tz": tz}},
                [
                    f"Invalid UTC offset `{tz}`: expecting `+HH:MM` or `-HH:MM`",
                    _INVALID_TZ_MSG,
                ],
                id=f"bad-offset-{tz}",
            )
            for tz in (
                "+5:00",
                "-0400",
                "+05:3",
                "+05:300",
                "+24:00",
                "-05:60",
                "+0a:00",
                "+05-30",
                "+",
            )
        ],
        pytest.param(
            {"ts-format": {"tz": "-00:00"}},
            [
                "Invalid UTC offset `-00:00`: use `+00:00` or `utc`.",
                _INVALID_TZ_MSG,
            ],
            id="neg-zero-offset",
        ),
        pytest.param(
            {"ts-format": {"tz": ""}},
            ["Empty time zone string.", _INVALID_TZ_MSG],
            id="empty-tz",
        ),
        pytest.param(
            {"ts-format": {"tz": "Mars/Olympus_Mons"}},
            ["Unknown time zone `Mars/Olympus_Mons`.", _INVALID_TZ_MSG],
            id="unknown-tz-id",
        ),
        *[
            pytest.param(
                {"ts-format": ts_format, "gmt": gmt},
                "`ts-format` and `gmt` parameters are mutually exclusive.",
                id=f"with-gmt-{str(gmt).lower()}-{ts_format_id}",
            )
            for gmt in (True, False)
            for ts_format, ts_format_id in (({"tz": "utc"}, "tz"), ({}, "empty"))
        ],
        pytest.param(
            {"ts-format": {"tz": "+01:00"}, "begin": "1970-01-01 00:30"},
            "Cannot convert date/time to nanoseconds from origin (date/time before "
            'the Unix epoch or after 2262?): param="1970-01-01 00:30"',
            id="before-epoch-offset",
        ),
        pytest.param(
            {"ts-format": {"tz": "utc"}, "end": "1969-12-31 23:00"},
            "Cannot convert date/time to nanoseconds from origin (date/time before "
            'the Unix epoch or after 2262?): param="1969-12-31 23:00"',
            id="before-epoch-utc",
        ),
    ],
)
def test_tz_invalid(
    trimmer_comp_cls, params: Dict[str, Any], expected_msgs: Union[str, List[str]]
):
    graph = bt2.Graph()

    if isinstance(expected_msgs, str):
        expected_msgs = [expected_msgs]

    with btu.raises_bt2_error_with_causes(*expected_msgs):
        graph.add_component(trimmer_comp_cls, "flt", params=params)


# Error cause message of a date/time which the component can't convert
# to nanoseconds from origin, followed with `param="..."` or
# `date-time=...`.
_CANNOT_CONVERT_MSG = (
    "Cannot convert date/time to nanoseconds from origin (date/time before "
    "the Unix epoch or after 2262?): "
)


# Tests the `begin` and `end` parameters around the greatest date/time
# of which the number of nanoseconds from origin fits a signed 64-bit
# integer (2262-04-11 23:47:16.854775807 UTC), with a local time zone
# at UTC.
#
# A time of day becomes a date/time on the date of the first message
# (2262-04-11).
@pytest.mark.needs_64_bit_time_t
@pytest.mark.parametrize(
    ["params", "expected_msg"],
    [
        pytest.param({"end": "2262-04-11 23:47:16.854775807"}, None, id="max"),
        pytest.param({"begin": "23:47:16.854775807"}, None, id="time-of-day-max"),
        pytest.param(
            {"begin": "2262-04-11 23:47:16.854775808"},
            _CANNOT_CONVERT_MSG + 'param="2262-04-11 23:47:16.854775808"',
            id="after-max",
        ),
        pytest.param(
            {"end": "9999-12-31 23:59"},
            _CANNOT_CONVERT_MSG + 'param="9999-12-31 23:59"',
            id="far-after-max",
        ),
        pytest.param(
            {"begin": "23:47:16.854775808"},
            _CANNOT_CONVERT_MSG + "date-time=2262-04-11",
            id="time-of-day-after-max",
        ),
    ],
)
def test_tz_after_max(
    trimmer_comp_cls,
    set_local_tz,
    params: Dict[str, Any],
    expected_msg: Optional[str],
):
    # See test_tz_local()
    set_local_tz("UTC0")
    times = ["2262-04-11T23:00", "2262-04-11T23:47"]

    if expected_msg is None:
        assert _trimmed_events(trimmer_comp_cls, times, params) == (
            times if "end" in params else []
        )
        return

    with btu.raises_bt2_error_with_causes(expected_msg):
        _trimmed_events(trimmer_comp_cls, times, params)


# Tests RFC 3339 date-times as the `begin` and `end` parameters.
@pytest.mark.parametrize(
    ["begin", "end", "expected"],
    [
        pytest.param(
            "2024-07-01T12:00:00Z", "2024-07-01T13:30:00Z", _HOURLY[2:4], id="z"
        ),
        pytest.param(
            "2024-07-01t12:00:00z",
            "2024-07-01t13:30:00z",
            _HOURLY[2:4],
            id="lowercase-t-z",
        ),
        pytest.param(
            "2024-07-01 12:00:00Z",
            "2024-07-01 13:30:00Z",
            _HOURLY[2:4],
            id="space-separator",
        ),
        pytest.param(
            "2024-07-01T12:00:00+00:00",
            "2024-07-01T13:30:00+00:00",
            _HOURLY[2:4],
            id="offset-pos-zero",
        ),
        # RFC 3339, 4.3: UTC, unknown local offset
        pytest.param(
            "2024-07-01T12:00:00-00:00",
            "2024-07-01T13:30:00-00:00",
            _HOURLY[2:4],
            id="offset-neg-zero",
        ),
        pytest.param(
            "2024-07-01T08:00:00-04:00",
            "2024-07-01T09:30:00-04:00",
            _HOURLY[2:4],
            id="offset-neg",
        ),
        pytest.param(
            "2024-07-01T17:30:00+05:30",
            "2024-07-01T19:00:00+05:30",
            _HOURLY[2:4],
            id="offset-pos-half-hour",
        ),
        pytest.param(
            "2024-07-02T02:00:00+14:00",
            "2024-07-02T03:30:00+14:00",
            _HOURLY[2:4],
            id="offset-next-day",
        ),
        pytest.param(
            "2024-06-30T23:00:00-11:00",
            "2024-07-01T00:30:00-11:00",
            _HOURLY[0:2],
            id="offset-prev-day",
        ),
        pytest.param(
            "2024-07-01T08:00:00-04:00",
            "2024-07-01T19:00:00+05:30",
            _HOURLY[2:4],
            id="different-offsets",
        ),
        pytest.param(
            "2024-07-01T11:59:59.999999999Z",
            "2024-07-01T13:00:00.000000001Z",
            _HOURLY[2:4],
            id="ns-around",
        ),
        pytest.param(
            "2024-07-01T12:00:00.000000001Z",
            "2024-07-01T12:59:59.999999999Z",
            [],
            id="ns-within",
        ),
        pytest.param(
            "2024-07-01T12:00:00.5Z",
            "2024-07-01T13:00:00.0-00:00",
            _HOURLY[3:4],
            id="ns-partial",
        ),
        # A leap second means the first second of the next minute
        pytest.param(
            "2024-07-01T11:59:60Z",
            "2024-07-01T12:59:60Z",
            _HOURLY[2:4],
            id="leap-second",
        ),
        pytest.param(
            "1969-12-31T23:00:00Z",
            "2024-07-01T12:00:00Z",
            _HOURLY[0:3],
            id="before-epoch",
        ),
        # Smallest date-time which fits as nanoseconds from origin
        pytest.param(
            "1677-09-21T00:12:43.145224192Z",
            None,
            _HOURLY,
            id="min",
            marks=[
                pytest.mark.needs_64_bit_time_t,
                pytest.mark.skipif(
                    btu.os_type() == btu.OsType.DARWIN,
                    reason="timegm() of macOS rejects years before 1900",
                ),
                pytest.mark.skipif(
                    btu.os_type() == btu.OsType.MINGW,
                    reason="_mkgmtime() of Windows rejects years before 1969",
                ),
            ],
        ),
        # Greatest date-time which fits as nanoseconds from origin
        pytest.param(
            None,
            "2262-04-11T23:47:16.854775807Z",
            _HOURLY,
            id="max",
            marks=pytest.mark.needs_64_bit_time_t,
        ),
        pytest.param(
            "2024-07-01T08:00:00-04:00",
            str(_ns_from_utc("2024-07-01T13:30") // 1_000_000_000),
            _HOURLY[2:4],
            id="with-sec",
        ),
        pytest.param(
            "2024-07-01 12:00",
            "2024-07-01T13:30:00Z",
            _HOURLY[2:4],
            id="with-date-time",
        ),
    ],
)
def test_rfc_3339(
    trimmer_comp_cls, begin: Optional[str], end: Optional[str], expected: List[str]
):
    params = {"ts-format": {"tz": "utc"}}

    if begin is not None:
        params["begin"] = begin

    if end is not None:
        params["end"] = end

    assert _trimmed_events(trimmer_comp_cls, _HOURLY, params) == expected


# Tests that the component converts an RFC 3339 date-time to the
# exact nanoseconds from origin, the source having an event one minute
# before, at, and one minute after midnight UTC on the date `date`.
@pytest.mark.parametrize(
    "date",
    [
        "1970-01-01",
        "1970-03-01",
        "1972-02-29",
        "1972-03-01",
        "1999-12-31",
        "2000-02-29",
        "2000-03-01",
        "2024-07-01",
        *[
            pytest.param(date, marks=pytest.mark.needs_64_bit_time_t)
            for date in ("2100-02-28", "2100-03-01", "2262-04-11")
        ],
    ],
)
def test_rfc_3339_exact(trimmer_comp_cls, date: str):
    midnight = _ns_from_utc(f"{date}T00:00")
    times = [_event_name(midnight + delta * 60_000_000_000) for delta in (-1, 0, 1)]

    # Before the Unix epoch: can't have such an event
    if date == "1970-01-01":
        times = times[1:]

    assert _trimmed_events(
        trimmer_comp_cls,
        times,
        {"begin": f"{date}T00:00:00Z", "end": f"{date}T00:00:00Z"},
    ) == [f"{date}T00:00"]


# Tests that the `tz` entry of the `ts-format` parameter, the `gmt`
# parameter, and the local time zone have no effect on an RFC 3339
# date-time.
@pytest.mark.parametrize(
    "params",
    [
        pytest.param({}, id="default"),
        pytest.param({"gmt": True}, id="gmt"),
        *[
            pytest.param({"ts-format": {"tz": tz}}, id=f"tz-{tz}")
            for tz in ("local", "utc", "+05:30", "-11:00", "+14:00")
        ],
    ],
)
def test_rfc_3339_tz(trimmer_comp_cls, set_local_tz, params: Dict[str, Any]):
    # See test_tz_local()
    set_local_tz("XYZ-3")
    assert (
        _trimmed_events(
            trimmer_comp_cls,
            _HOURLY,
            {
                **params,
                "begin": "2024-07-01T08:00:00-04:00",
                "end": "2024-07-01T13:30:00Z",
            },
        )
        == _HOURLY[2:4]
    )


# Tests that the component rejects invalid RFC 3339 date-times with the
# expected error cause message.
@pytest.mark.parametrize(
    ["begin", "expected_msg"],
    [
        pytest.param("2024-02-30T12:00:00Z", "Invalid date", id="bad-date"),
        pytest.param("2024-07-01T24:00:00Z", "Invalid time of day", id="bad-hour"),
        pytest.param("2024-07-01T12:60:00Z", "Invalid time of day", id="bad-minute"),
        pytest.param("2024-07-01T12:00:61Z", "Invalid time of day", id="bad-second"),
        pytest.param(
            "2024-07-01T12:00:00+24:00", "Invalid UTC offset", id="bad-offset-hour"
        ),
        pytest.param(
            "2024-07-01T12:00:00-05:60", "Invalid UTC offset", id="bad-offset-minute"
        ),
        *[
            pytest.param(
                begin,
                "Date/time is out of range (nanoseconds from origin)",
                id=f"out-of-range-{begin_id}",
            )
            for begin, begin_id in (
                ("9999-12-31T23:59:59Z", "far-future"),
                ("2262-04-11T23:47:16.854775808Z", "max-plus-one"),
                ("1677-09-21T00:12:43.145224191Z", "min-minus-one"),
                ("0000-01-01T00:00:00Z", "far-past"),
            )
        ],
        *[
            pytest.param(begin, "Invalid date/time format", id=f"bad-format-{begin_id}")
            for begin, begin_id in (
                ("2024-07-01T12:00Z", "no-second"),
                ("2024-07-01T12:00:00", "no-offset"),
                ("2024-07-01T12:00:00.Z", "empty-fraction"),
                ("2024-07-01T12:00:00.1234567890Z", "too-many-ns-digits"),
                ("2024-07-01T12:00:00+0500", "offset-no-colon"),
                ("2024-07-01T12:00:00+05", "offset-no-minute"),
                ("2024-07-01T12:00:00+5:00", "offset-one-hour-digit"),
                ("2024-07-01T12:00:00 Z", "space-before-offset"),
                ("2024-07-01X12:00:00Z", "bad-separator"),
                ("2024-07-01TT12:00:00Z", "double-separator"),
                ("12:00:00Z", "no-date"),
            )
        ],
    ],
)
def test_rfc_3339_invalid(trimmer_comp_cls, begin: str, expected_msg: str):
    graph = bt2.Graph()

    with btu.raises_bt2_error_with_causes(f'{expected_msg}: param="{begin}"'):
        graph.add_component(trimmer_comp_cls, "flt", params={"begin": begin})


# Number of seconds from origin of 12:00 and 13:30 UTC on 1 July 2024.
_SEC_12_00 = _ns_from_utc("2024-07-01T12:00") // 1_000_000_000
_SEC_13_30 = _ns_from_utc("2024-07-01T13:30") // 1_000_000_000


# `begin` and `end` parameters, for each format, which make a trimmer
# let `_HOURLY[2:4]` (`_HOURLY` for `date`) through with the `utc`
# time zone.
_TYPE_FMT_BOUNDS = {
    "date-time": ("2024-07-01 12:00", "2024-07-01 13:30"),
    "date": ("2024-07-01", "2024-07-02"),
    "rfc-3339": ("2024-07-01T08:00:00-04:00", "2024-07-01T13:30:00Z"),
    "time": ("12:00", "13:30"),
    "sec-str": (str(_SEC_12_00), str(_SEC_13_30)),
    "sec-int": (_SEC_12_00, _SEC_13_30),
}

# Formats of `_TYPE_FMT_BOUNDS` which each value of the `type` entry of
# the `ts-format` parameter accepts.
_TYPE_ACCEPTED_FMTS = {
    "auto": set(_TYPE_FMT_BOUNDS),
    "date-time": {"date-time", "date", "time"},
    "seconds-from-origin": {"sec-str", "sec-int"},
    "rfc-3339": {"rfc-3339"},
}


# Tests that the `type` entry of the `ts-format` parameter makes the
# component accept the `begin` and `end` parameters having its format.
@pytest.mark.parametrize(
    ["ts_fmt_type", "fmt"],
    [
        pytest.param(ts_fmt_type, fmt, id=f"{ts_fmt_type}-{fmt}")
        for ts_fmt_type, fmts in _TYPE_ACCEPTED_FMTS.items()
        for fmt in sorted(fmts)
    ],
)
def test_ts_fmt_type(trimmer_comp_cls, ts_fmt_type: str, fmt: str):
    begin, end = _TYPE_FMT_BOUNDS[fmt]
    ts_fmt: Dict[str, Any] = {"type": ts_fmt_type}

    # The `tz` entry only applies to the `auto` and `date-time` types
    if ts_fmt_type in ("auto", "date-time"):
        ts_fmt["tz"] = "utc"

    assert _trimmed_events(
        trimmer_comp_cls,
        _HOURLY,
        {"ts-format": ts_fmt, "begin": begin, "end": end},
    ) == (_HOURLY if fmt == "date" else _HOURLY[2:4])


# Tests that the `tz` entry of the `ts-format` parameter applies to a
# date/time and to a time with the `date-time` type.
@pytest.mark.parametrize(
    ["begin", "end"],
    [
        pytest.param("2024-07-01 17:30", "2024-07-01 19:00", id="date-time"),
        pytest.param("17:30", "19:00", id="time"),
    ],
)
def test_ts_fmt_type_date_time_tz(trimmer_comp_cls, begin: str, end: str):
    assert (
        _trimmed_events(
            trimmer_comp_cls,
            _HOURLY,
            {
                "ts-format": {"type": "date-time", "tz": "+05:30"},
                "begin": begin,
                "end": end,
            },
        )
        == _HOURLY[2:4]
    )


# Tests that the component rejects the `begin` or `end` parameter when
# its format isn't the one of the `type` entry of the
# `ts-format` parameter.
@pytest.mark.parametrize("param_name", ["begin", "end"])
@pytest.mark.parametrize(
    ["ts_fmt_type", "fmt"],
    [
        pytest.param(ts_fmt_type, fmt, id=f"{ts_fmt_type}-{fmt}")
        for ts_fmt_type, fmts in _TYPE_ACCEPTED_FMTS.items()
        for fmt in sorted(set(_TYPE_FMT_BOUNDS) - fmts)
    ],
)
def test_ts_fmt_type_mismatch(
    trimmer_comp_cls, param_name: str, ts_fmt_type: str, fmt: str
):
    val = _TYPE_FMT_BOUNDS[fmt][0]
    graph = bt2.Graph()

    expected_msg = (
        f"Invalid `{ts_fmt_type}` format (`type` entry of `ts-format` parameter): "
        f'param="{val}"'
    )

    with btu.raises_bt2_error_with_causes(expected_msg):
        graph.add_component(
            trimmer_comp_cls,
            "flt",
            params={"ts-format": {"type": ts_fmt_type}, param_name: val},
        )


# Tests that the component rejects invalid `type` entries of the
# `ts-format` parameter, as well as a `tz` entry with a type to which it
# doesn't apply.
@pytest.mark.parametrize(
    ["ts_fmt", "expected_msg"],
    [
        pytest.param(
            {"type": 23},
            "Error validating parameter `ts-format.type`: unexpected type",
            id="type-not-str",
        ),
        *[
            pytest.param(
                {"type": ts_fmt_type},
                "Error validating parameter `ts-format.type`: string is not "
                f"amongst the available choices: string={ts_fmt_type}, "
                "choices=[auto, date-time, seconds-from-origin, rfc-3339]",
                id=f"unknown-type-{ts_fmt_type}",
            )
            for ts_fmt_type in ("time", "cycles", "AUTO", "")
        ],
        *[
            pytest.param(
                {"type": ts_fmt_type, "tz": "utc"},
                "`tz` entry of `ts-format` parameter only applies to the `auto` "
                f'and `date-time` types: type="{ts_fmt_type}"',
                id=f"tz-with-{ts_fmt_type}",
            )
            for ts_fmt_type in ("seconds-from-origin", "rfc-3339")
        ],
    ],
)
def test_ts_fmt_type_invalid(
    trimmer_comp_cls, ts_fmt: Dict[str, Any], expected_msg: str
):
    graph = bt2.Graph()

    with btu.raises_bt2_error_with_causes(expected_msg):
        graph.add_component(trimmer_comp_cls, "flt", params={"ts-format": ts_fmt})
