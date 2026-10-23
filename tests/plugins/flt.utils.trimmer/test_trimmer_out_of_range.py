# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import types

import bt2
import pytest
import bt_tests_utils as btu


def _assert_has_cause(exc: bt2._Error, expected_msg: str):
    assert any(expected_msg in cause.message for cause in exc), [
        cause.message for cause in exc
    ]


# Tests that a `flt.utils.trimmer` component rejects a `begin` or `end`
# date/time (UTC) of which the number of nanoseconds from origin is
# negative or doesn't fit a signed 64-bit integer.
@pytest.mark.parametrize(
    ["param_name", "date_time"],
    [
        pytest.param("begin", "1969-12-31 23:00", id="before-epoch-begin"),
        pytest.param("end", "1969-12-31 23:00:59.5", id="before-epoch-end"),
        pytest.param("begin", "1900-01-01", id="before-epoch-date"),
        pytest.param("end", "2262-04-11 23:47:17", id="after-max"),
    ],
)
def test_out_of_range(
    trimmer_comp_cls: bt2._FilterComponentClassConst,
    param_name: str,
    date_time: str,
):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.Graph().add_component(
            trimmer_comp_cls, "flt", params={"gmt": True, param_name: date_time}
        )

    _assert_has_cause(
        exc_info.value,
        "Cannot convert date/time to nanoseconds from origin (date/time before "
        f'the Unix epoch or after 2262?): param="{date_time}"',
    )


# Tests that a `flt.utils.trimmer` component rejects a `begin` or `end`
# `[-]SEC[.NANO]` time of which the absolute value of the number of
# nanoseconds from origin doesn't fit a signed 64-bit integer, even
# when the number of seconds itself doesn't fit an unsigned
# 64-bit integer.
@pytest.mark.parametrize(
    ["param_name", "time"],
    [
        pytest.param("begin", "9223372037", id="sec-begin"),
        pytest.param("end", "-9223372037", id="neg-sec-end"),
        pytest.param("begin", "9223372036.854775808", id="sec-nano-begin"),
        pytest.param("end", "-9223372036.854775808", id="neg-sec-nano-end"),
        pytest.param("begin", "123456789012345678901234567890", id="many-digits-begin"),
        pytest.param(
            "end", "-123456789012345678901234567890.5", id="neg-many-digits-end"
        ),
    ],
)
def test_sec_out_of_range(
    trimmer_comp_cls: bt2._FilterComponentClassConst,
    param_name: str,
    time: str,
):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.Graph().add_component(trimmer_comp_cls, "flt", params={param_name: time})

    _assert_has_cause(
        exc_info.value,
        f'Time is out of range (nanoseconds from origin): param="{time}"',
    )


# Tests that a `flt.utils.trimmer` component accepts the largest number
# of seconds from origin of which the number of nanoseconds from origin
# fits a signed 64-bit integer.
def test_secs_max(trimmer_comp_cls: bt2._FilterComponentClassConst):
    bt2.Graph().add_component(
        trimmer_comp_cls, "flt", params={"end": "9223372036.854775807"}
    )


class _BeforeEpochMsgIter(bt2._UserMessageIterator):
    def __init__(self, config, port):
        stream = port.user_data.tc().create_stream(port.user_data.sc)

        # The default clock class has an offset of -3600 seconds from
        # the Unix epoch, therefore this event message is at
        # 1969-12-31 23:00:00 UTC.
        self._msgs = [
            self._create_stream_beginning_message(stream),
            self._create_event_message(port.user_data.ec, stream, 0),
            self._create_stream_end_message(stream),
        ]

    def __next__(self):
        if len(self._msgs) == 0:
            raise StopIteration

        return self._msgs.pop(0)


class _BeforeEpochSrc(
    bt2._UserSourceComponent, message_iterator_class=_BeforeEpochMsgIter
):
    def __init__(self, config, params, obj):
        tc = self._create_trace_class()
        sc = tc.create_stream_class(
            default_clock_class=self._create_clock_class(
                frequency=1, offset=bt2.ClockOffset(-3600)
            )
        )
        self._add_output_port(
            "out",
            types.SimpleNamespace(tc=tc, sc=sc, ec=sc.create_event_class(name="ev")),
        )


# Tests that a `flt.utils.trimmer` message iterator fails when it can't
# convert a `begin` or `end` time without a date to a number of
# nanoseconds from origin because the date of the first message is
# before the Unix epoch.
@pytest.mark.parametrize("param_name", ["begin", "end"])
def test_iter_before_epoch(
    trimmer_comp_cls: bt2._FilterComponentClassConst,
    dummy_comp_cls: bt2._SinkComponentClassConst,
    param_name: str,
):
    graph = bt2.Graph()
    src_comp = graph.add_component(_BeforeEpochSrc, "src")
    flt_comp = graph.add_component(
        trimmer_comp_cls, "flt", params={"gmt": True, param_name: "12:00"}
    )
    sink_comp = graph.add_component(dummy_comp_cls, "sink")
    graph.connect_ports(src_comp.output_ports["out"], flt_comp.input_ports["in"])
    graph.connect_ports(flt_comp.output_ports["out"], sink_comp.input_ports["in"])

    with pytest.raises(bt2._Error) as exc_info:
        graph.run()

    _assert_has_cause(
        exc_info.value,
        "Cannot convert date/time to nanoseconds from origin (date/time before "
        "the Unix epoch or after 2262?): date-time=1969-12-31T12:00:00.000000000",
    )


# Number of seconds from the Unix epoch of 2040-01-01 00:00:00 UTC,
# which doesn't fit a signed 32-bit integer.
_SEC_2040 = 2_208_988_800


class _After2038MsgIter(bt2._UserMessageIterator):
    def __init__(self, config, port):
        stream = port.user_data.tc().create_stream(port.user_data.sc)

        # The default clock class has an offset of `_SEC_2040` seconds
        # from the Unix epoch, therefore those event messages are at
        # 2040-01-01 00:00:00 and 13:00:00 UTC.
        self._msgs = [
            self._create_stream_beginning_message(stream),
            self._create_event_message(port.user_data.ec, stream, 0),
            self._create_event_message(port.user_data.ec, stream, 13 * 3600),
            self._create_stream_end_message(stream),
        ]
        self._at = 0

        # The trimmer message iterator seeks its upstream message
        # iterator to the beginning time
        config.can_seek_forward = True

    def _user_seek_beginning(self):
        self._at = 0

    def __next__(self):
        if self._at == len(self._msgs):
            raise StopIteration

        self._at += 1
        return self._msgs[self._at - 1]


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


# Tests that a `flt.utils.trimmer` message iterator converts a `begin`
# time without a date to a number of nanoseconds from origin on the
# date of the first message (after 2038) when its number of seconds fits
# `time_t`, otherwise that it fails.
@pytest.mark.parametrize(
    "fits_time_t",
    [
        pytest.param(True, marks=pytest.mark.needs_64_bit_time_t, id="64-bit-time-t"),
        pytest.param(False, marks=pytest.mark.needs_32_bit_time_t, id="32-bit-time-t"),
    ],
)
def test_iter_after_2038(
    trimmer_comp_cls: bt2._FilterComponentClassConst, fits_time_t: bool
):
    def event_ns():
        msg_it = bt2.TraceCollectionMessageIterator(
            [bt2.ComponentSpec(_After2038Src)],
            [
                bt2.ComponentSpec(
                    trimmer_comp_cls, params={"gmt": True, "begin": "12:00"}
                )
            ],
            plugin_set=btu.plugin_set(),
        )

        return [
            msg.default_clock_snapshot.ns_from_origin
            for msg in msg_it
            if isinstance(msg, bt2._EventMessageConst)
        ]

    if fits_time_t:
        # Only the 2040-01-01 13:00:00 UTC event message remains
        assert event_ns() == [(_SEC_2040 + 13 * 3600) * 1_000_000_000]
        return

    with pytest.raises(bt2._Error) as exc_info:
        event_ns()

    _assert_has_cause(
        exc_info.value,
        "Cannot convert timestamp to date and time (number of seconds doesn't "
        "fit `time_t`): ts=2208988800",
    )
