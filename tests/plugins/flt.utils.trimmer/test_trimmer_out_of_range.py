# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import types

import bt2
import pytest


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
# number of seconds from origin of which the number of nanoseconds from
# origin doesn't fit a signed 64-bit integer.
@pytest.mark.parametrize(
    ["param_name", "secs"],
    [
        pytest.param("begin", "9223372037", id="over-max-s"),
        pytest.param("end", "9223372036.854775808", id="over-max-ns"),
        pytest.param("begin", "-9223372036.854775808", id="under-min-ns"),
        pytest.param("end", "18446744074", id="over-max-u64-s"),
    ],
)
def test_secs_out_of_range(
    trimmer_comp_cls: bt2._FilterComponentClassConst,
    param_name: str,
    secs: str,
):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.Graph().add_component(trimmer_comp_cls, "flt", params={param_name: secs})

    _assert_has_cause(
        exc_info.value,
        "Number of nanoseconds from origin doesn't fit a signed 64-bit integer: "
        f'param="{secs}"',
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
        "the Unix epoch or after 2262?): date-time=1969-12-31 12:00:00.000000000",
    )
