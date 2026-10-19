# SPDX-FileCopyrightText: 2020 EfficiOS, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import types
import pathlib
from typing import Any, Dict, List, Optional

import bt2
import pytest
import bt_tests_utils as btu
import bt_tests_cli_utils as btu_cli


# Tests that the component returns an error if the graph is configured
# while the input port of the component is left disconnected.
def test_unconnected_port_raises(pretty_comp_cls):
    graph = bt2.Graph()
    graph.add_component(pretty_comp_cls, "snk")

    with pytest.raises(bt2._Error) as exc_info:
        graph.run()

    assert (
        'Single input port is not connected: port-name="in"'
        in exc_info.value[0].message
    )


# Tests that the component appends an error cause which describes an
# invalid initialization parameter.
def test_invalid_param_error_cause(pretty_comp_cls):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.Graph().add_component(pretty_comp_cls, "snk", params={"clock-gmt": "yes"})

    assert any(
        "Error validating parameter `clock-gmt`: unexpected type: "
        "expected-type=BOOL, actual-type=STRING" in cause.message
        for cause in exc_info.value
    )


# Tests that the component emits the right terminal color codes when
# asked to.
def test_term_colors(build_root_dir, ctf_traces_dir):
    result = btu_cli.run_cli(
        build_root_dir,
        [
            str(ctf_traces_dir / "2/succeed/smalltrace"),
            "-c",
            "sink.text.pretty",
            btu_cli.CliParams({"color": "always"}),
        ],
        check=True,
        extra_env={
            "BABELTRACE_TERM_COLOR_BRIGHT_MEANS_BOLD": "0",
            "TERM": "xterm-256color",
        },
    )

    assert result.stdout == (
        "\x1b[1m\x1b[95mstring\x1b[0m: { \x1b[36mstr\x1b[0m = "
        '\x1b[1m"This is a test trace"\x1b[0m }\n'
        "\x1b[1m\x1b[95mstring\x1b[0m: { \x1b[36mstr\x1b[0m = "
        '\x1b[1m"with only two small events."\x1b[0m }\n'
    )


# Number of seconds from the Unix epoch of 2040-01-01 00:00:00 UTC,
# which doesn't fit a signed 32-bit integer.
_SEC_2040 = 2_208_988_800


class _After2038MsgIter(bt2._UserMessageIterator):
    def __init__(self, config, port):
        stream = port.user_data.tc().create_stream(port.user_data.sc)

        # The default clock class has an offset of `_SEC_2040` seconds
        # from the Unix epoch, therefore this event message is at
        # 2040-01-01 00:00:00 UTC.
        self._msgs = [
            self._create_stream_beginning_message(stream),
            self._create_event_message(port.user_data.ec, stream, 0),
            self._create_stream_end_message(stream),
        ]

    def __next__(self):
        if len(self._msgs) == 0:
            raise StopIteration

        return self._msgs.pop(0)


class _After2038Src(bt2._UserSourceComponent, message_iterator_class=_After2038MsgIter):
    def __init__(self, config, params, obj):
        tc = self._create_trace_class()
        sc = tc.create_stream_class(
            default_clock_class=self._create_clock_class(
                frequency=1, offset=bt2.ClockOffset(_SEC_2040)
            )
        )
        self._add_output_port(
            "out",
            types.SimpleNamespace(tc=tc, sc=sc, ec=sc.create_event_class(name="ev")),
        )


# Tests the timestamp of an event message after 2038 with the UTC
# date/time format: the component writes the date/time when the number
# of seconds fits `time_t`, otherwise it falls back to the seconds from
# origin format.
@pytest.mark.parametrize(
    "expected",
    [
        pytest.param(
            "[2040-01-01 00:00:00.000000000] ev:",
            marks=pytest.mark.needs_64_bit_time_t,
            id="64-bit-time-t",
        ),
        pytest.param(
            "[2208988800.000000000] ev:",
            marks=pytest.mark.needs_32_bit_time_t,
            id="32-bit-time-t",
        ),
    ],
)
def test_ts_after_2038(pretty_comp_cls, tmp_path: pathlib.Path, expected: str):
    out_path = tmp_path / "output.txt"
    btu.convert(
        bt2.ComponentSpec(_After2038Src),
        btu.SinkComponentSpec(
            pretty_comp_cls,
            {
                "path": str(out_path),
                "color": "never",
                "clock-date": True,
                "clock-gmt": True,
                "no-delta": True,
            },
        ),
    )

    assert out_path.read_text(encoding="utf-8").strip() == expected


# Runs the CLI to pretty-print the trace of `mctf_name` with the
# `sink.text.pretty` initialization parameters `params` (`no-delta` and
# `color` added) and returns the lines it writes.
def _pretty_lines(
    build_root_dir: pathlib.Path,
    materialize_mctf,
    mctf_name: str,
    params: Dict[str, Any],
    extra_env: Optional[Dict[str, str]] = None,
) -> List[str]:
    trace = materialize_mctf(pathlib.Path(__file__).parent / "data" / mctf_name)

    result = btu_cli.run_cli(
        build_root_dir,
        [
            str(trace.path),
            "-c",
            "sink.text.pretty",
            btu_cli.CliParams({"color": "never", "no-delta": True, **params}),
        ],
        check=True,
        extra_env=extra_env,
    )

    return result.stdout.splitlines()


# Tests the `local` value of the `tz` entry of the `ts-format`
# parameter, as well as the default time zone, setting the `TZ`
# environment variable of the CLI process.
@pytest.mark.parametrize(
    ["tz_env", "ts_format", "expected"],
    [
        pytest.param(
            # POSIX offsets are negated: this is UTC+05:30
            "IST-5:30",
            {"type": "date-time", "tz": "local"},
            [
                "[2012-01-15 17:30:00.000000000] e: { v = 0xAB }",
                "[2012-07-15 17:30:00.000000000] e: { v = 0xAB }",
            ],
            id="date-time-local",
        ),
        pytest.param(
            # POSIX time zone: UTC−05:00, daylight saving time
            # (UTC−04:00) from the second Sunday of March to the first
            # Sunday of Movember.
            "EST5EDT,M3.2.0,M11.1.0",
            {"type": "date-time"},
            [
                "[2012-01-15 07:00:00.000000000] e: { v = 0xAB }",
                "[2012-07-15 08:00:00.000000000] e: { v = 0xAB }",
            ],
            id="date-time",
        ),
        pytest.param(
            "EST5EDT,M3.2.0,M11.1.0",
            {},
            [
                "[07:00:00.000000000] e: { v = 0xAB }",
                "[08:00:00.000000000] e: { v = 0xAB }",
            ],
            id="time",
        ),
        pytest.param(
            # `TZ` doesn't apply
            "EST5EDT,M3.2.0,M11.1.0",
            {"type": "time", "tz": "utc"},
            [
                "[12:00:00.000000000] e: { v = 0xAB }",
                "[12:00:00.000000000] e: { v = 0xAB }",
            ],
            id="time-utc",
        ),
    ],
)
def test_ts_format_tz_local(
    build_root_dir,
    materialize_mctf,
    tz_env: str,
    ts_format: Dict[str, str],
    expected,
):
    params = {"ts-format": ts_format} if ts_format else {}

    assert (
        _pretty_lines(
            build_root_dir,
            materialize_mctf,
            "jan-jul.mctf",
            params,
            extra_env={"TZ": tz_env},
        )
        == expected
    )


# Tests that the component rejects invalid `ts-format`-related
# initialization parameters with the expected error cause message.
@pytest.mark.parametrize(
    ["params", "expected_msg"],
    [
        pytest.param(
            {"ts-format": "rfc-3339"},
            "Error validating parameter `ts-format`: unexpected type: "
            "expected-type=MAP, actual-type=STRING",
            id="not-map",
        ),
        pytest.param(
            {"ts-format": {"type": "iso-8601"}},
            "Error validating parameter `ts-format.type`: string is not amongst "
            "the available choices",
            id="bad-type",
        ),
        pytest.param(
            {"ts-format": {"type": 23}},
            "Error validating parameter `ts-format.type`: unexpected type",
            id="type-not-str",
        ),
        pytest.param(
            {"ts-format": {"tz": True}},
            "Error validating parameter `ts-format.tz`: unexpected type",
            id="tz-not-str",
        ),
        pytest.param(
            {"ts-format": {"type": "time", "zone": "utc"}},
            "Error validating parameter `ts-format`: unexpected key `zone`.",
            id="unknown-key",
        ),
        pytest.param(
            {"ts-format": {"type": "date-time", "tz": "Mars/Olympus_Mons"}},
            "Error validating parameter `ts-format.tz`: string is not amongst "
            "the available choices",
            id="unknown-tz-id",
        ),
        pytest.param(
            {"ts-format": {"type": "cycles", "tz": "utc"}},
            "`tz` entry of `ts-format` parameter only applies to the `time` and "
            '`date-time` types: type="cycles"',
            id="tz-with-cycles",
        ),
        pytest.param(
            {"ts-format": {"type": "seconds-from-origin", "tz": "utc"}},
            "`tz` entry of `ts-format` parameter only applies to the `time` and "
            '`date-time` types: type="seconds-from-origin"',
            id="tz-with-seconds-from-origin",
        ),
        *[
            pytest.param(
                {"ts-format": {"type": "date-time"}, legacy_param: False},
                f"`ts-format` and `{legacy_param}` parameters are mutually "
                "exclusive.",
                id=f"with-{legacy_param}",
            )
            for legacy_param in (
                "clock-cycles",
                "clock-seconds",
                "clock-date",
                "clock-gmt",
            )
        ],
    ],
)
def test_ts_format_invalid(pretty_comp_cls, params, expected_msg: str):
    graph = bt2.Graph()

    with pytest.raises(bt2._Error) as exc_info:
        graph.add_component(pretty_comp_cls, "snk", params=params)

    assert any(expected_msg in cause.message for cause in exc_info.value), [
        cause.message for cause in exc_info.value
    ]
