# SPDX-FileCopyrightText: 2020 EfficiOS, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import types
import pathlib

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
