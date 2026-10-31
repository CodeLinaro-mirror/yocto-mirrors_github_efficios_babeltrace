# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

from typing import Any, Dict, List, Tuple, Callable, Optional

import bt2
import pytest
import bt_tests_utils as btu


# Data of the output port of a `_Src` component.
class _SrcPortData:
    def __init__(
        self,
        create_msgs: Callable[[bt2._UserMessageIterator], List[bt2._MessageConst]],
        seek_try_again_count: int = 0,
    ):
        # Function which creates and returns the messages of a
        # `_SrcMsgIter` instance from the latter.
        self.create_msgs = create_msgs

        # Remaining number of times the "seek beginning" method of a
        # `_SrcMsgIter` instance raises `bt2.TryAgain`
        # before succeeding.
        self.seek_try_again_count = seek_try_again_count


# Message iterator of a `_Src` component: sends the messages which
# `create_msgs` of its port data returns.
class _SrcMsgIter(bt2._UserMessageIterator):
    def __init__(self, config, port):
        self._data = port.user_data
        self._msgs = self._data.create_msgs(self)
        self._at = 0
        config.can_seek_forward = True

    def _user_seek_beginning(self):
        if self._data.seek_try_again_count > 0:
            self._data.seek_try_again_count -= 1
            raise bt2.TryAgain

        self._at = 0

    def __next__(self):
        if self._at == len(self._msgs):
            raise StopIteration

        msg = self._msgs[self._at]
        self._at += 1
        return msg


# Source component of which the `obj` parameter is the data of its
# output port (`_SrcPortData`).
class _Src(bt2._UserSourceComponent, message_iterator_class=_SrcMsgIter):
    def __init__(self, config, params, obj):
        self._add_output_port("out", obj)


# Sink component which appends the messages it consumes to the
# list `obj`.
class _Sink(bt2._UserSinkComponent):
    def __init__(self, config, params, obj):
        self._msgs = obj
        self._add_input_port("in")

    def _user_graph_is_configured(self):
        self._msg_iter = self._create_message_iterator(self._input_ports["in"])

    def _user_consume(self):
        self._msgs.append(next(self._msg_iter))


# Runs a graph in which a `flt.utils.trimmer` component having the
# initialization parameters `trimmer_params` trims the messages of a
# `_Src` component having the output port data `port_data`, returning
# the messages it sends.
#
# Runs the graph again when it raises `bt2.TryAgain`.
def _trim(
    trimmer_comp_cls: bt2._FilterComponentClassConst,
    port_data: _SrcPortData,
    trimmer_params: Dict[str, Any],
) -> List[bt2._MessageConst]:
    msgs: List[bt2._MessageConst] = []
    graph = bt2.Graph()
    src = graph.add_component(_Src, "src", obj=port_data)
    flt = graph.add_component(trimmer_comp_cls, "flt", params=trimmer_params)
    sink = graph.add_component(_Sink, "sink", obj=msgs)
    graph.connect_ports(src.output_ports["out"], flt.input_ports["in"])
    graph.connect_ports(flt.output_ports["out"], sink.input_ports["in"])

    while True:
        try:
            graph.run()
            return msgs
        except bt2.TryAgain:
            pass


# Creates and returns, from the component of `msg_iter`, a stream and
# the class of its events.
#
# The class of the stream has a default clock class having the
# properties `cc_kwargs`, or none if `cc_kwargs` is empty.
def _create_stream(
    msg_iter: bt2._UserMessageIterator, **cc_kwargs: Any
) -> Tuple[bt2._Stream, bt2._EventClass]:
    comp = msg_iter._component
    tc = comp._create_trace_class()
    sc = tc.create_stream_class(
        default_clock_class=comp._create_clock_class(**cc_kwargs) if cc_kwargs else None
    )
    return tc().create_stream(sc), sc.create_event_class(name="ev")


# Returns the type name and the default clock snapshot value (or
# `None`) of `msg`.
def _msg_summary(msg: bt2._MessageConst) -> Tuple[str, Optional[int]]:
    if type(msg) is bt2._MessageIteratorInactivityMessageConst:
        return ("inactivity", msg.clock_snapshot.value)

    names = {
        bt2._StreamBeginningMessageConst: "stream-beginning",
        bt2._StreamEndMessageConst: "stream-end",
        bt2._EventMessageConst: "event",
    }

    cs = msg.default_clock_snapshot

    return (
        names[type(msg)],
        None if type(cs) is bt2._UnknownClockSnapshot else cs.value,
    )


# Tests that, when the end time of the trimming time range is infinite,
# a message iterator inactivity message doesn't end the
# trimming operation.
@pytest.mark.parametrize(
    "trimmer_params",
    [
        pytest.param({}, id="no-begin"),
        pytest.param({"begin": "0"}, id="begin"),
    ],
)
def test_inactivity_infinite_end(
    trimmer_comp_cls: bt2._FilterComponentClassConst, trimmer_params: Dict[str, Any]
):
    def create_msgs(msg_iter: bt2._UserMessageIterator):
        stream, ec = _create_stream(msg_iter, frequency=1)
        cc = stream.cls.default_clock_class

        return [
            msg_iter._create_stream_beginning_message(stream, 1),
            msg_iter._create_event_message(ec, stream, 2),
            msg_iter._create_message_iterator_inactivity_message(cc, 50),
            msg_iter._create_event_message(ec, stream, 100),
            msg_iter._create_stream_end_message(stream, 200),
        ]

    msgs = _trim(trimmer_comp_cls, _SrcPortData(create_msgs), trimmer_params)
    assert [_msg_summary(msg) for msg in msgs] == [
        ("stream-beginning", 1),
        ("event", 2),
        ("inactivity", 50),
        ("event", 100),
        ("stream-end", 200),
    ]


# Tests that a `flt.utils.trimmer` message iterator appends an error
# cause when a message has no default clock class.
@pytest.mark.parametrize(
    "trimmer_params",
    [
        pytest.param({}, id="no-begin"),
        pytest.param({"begin": "12:00", "gmt": True}, id="begin-time-of-day"),
    ],
)
def test_no_default_clock_class(
    trimmer_comp_cls: bt2._FilterComponentClassConst, trimmer_params: Dict[str, Any]
):
    def create_msgs(msg_iter: bt2._UserMessageIterator):
        stream, ec = _create_stream(msg_iter)

        return [
            msg_iter._create_stream_beginning_message(stream),
            msg_iter._create_event_message(ec, stream),
            msg_iter._create_stream_end_message(stream),
        ]

    with btu.raises_bt2_error_with_causes(
        "Unsupported message: stream class has no default clock class"
    ):
        _trim(trimmer_comp_cls, _SrcPortData(create_msgs), trimmer_params)


# Tests that a `flt.utils.trimmer` message iterator appends an error
# cause when it can't convert the clock snapshot of a message to
# nanoseconds from origin.
def test_clock_snapshot_ns_from_origin_overflow(
    trimmer_comp_cls: bt2._FilterComponentClassConst,
):
    def create_msgs(msg_iter: bt2._UserMessageIterator):
        # The number of nanoseconds from origin of this offset doesn't
        # fit a signed 64-bit integer.
        stream, ec = _create_stream(
            msg_iter, frequency=1, offset=bt2.ClockOffset(10**10)
        )

        return [
            msg_iter._create_stream_beginning_message(stream),
            msg_iter._create_event_message(ec, stream, 23),
            msg_iter._create_stream_end_message(stream),
        ]

    with btu.raises_bt2_error_with_causes(
        "Cannot convert clock snapshot to nanoseconds from origin: "
        "msg-type=Event, clock-snapshot-value=23"
    ):
        _trim(trimmer_comp_cls, _SrcPortData(create_msgs), {})


# Tests that a `flt.utils.trimmer` message iterator appends an error
# cause when it can't convert the end time of the trimming time range
# to a value of the default clock class of a stream to end.
def test_end_clock_value_overflow(trimmer_comp_cls: bt2._FilterComponentClassConst):
    def create_msgs(msg_iter: bt2._UserMessageIterator):
        # With this frequency, the end time of the trimming time range
        # (100 s) doesn't fit an unsigned 64-bit clock value.
        stream_fast, ec_fast = _create_stream(msg_iter, frequency=10**18, name="fast")
        stream_slow, ec_slow = _create_stream(msg_iter, frequency=1, name="slow")

        return [
            msg_iter._create_stream_beginning_message(stream_fast),
            msg_iter._create_stream_beginning_message(stream_slow),
            # 0.5 s: within the trimming time range
            msg_iter._create_event_message(ec_fast, stream_fast, 5 * 10**17),
            # 200 s: ends the trimming operation, therefore
            # also `stream_fast`.
            msg_iter._create_event_message(ec_slow, stream_slow, 200),
            msg_iter._create_stream_end_message(stream_slow),
            msg_iter._create_stream_end_message(stream_fast),
        ]

    with btu.raises_bt2_error_with_causes(
        "Cannot convert the end time of the trimming time range to a clock value: "
        "end-ns-from-origin=100000000000, clock-class-name=fast"
    ):
        _trim(trimmer_comp_cls, _SrcPortData(create_msgs), {"end": "100"})


# Tests that a `flt.utils.trimmer` message iterator, having found the
# date of a time of day bound of the trimming time range, makes its
# upstream message iterator seek again, without finding this date again,
# when this seeking operation raises `bt2.TryAgain`.
def test_seek_try_again(trimmer_comp_cls: bt2._FilterComponentClassConst):
    def create_msgs(msg_iter: bt2._UserMessageIterator):
        stream, ec = _create_stream(msg_iter, frequency=1)

        return [
            msg_iter._create_stream_beginning_message(stream),
            msg_iter._create_event_message(ec, stream, 0),
            msg_iter._create_event_message(ec, stream, 2),
            msg_iter._create_stream_end_message(stream),
        ]

    port_data = _SrcPortData(create_msgs, seek_try_again_count=1)

    # The date of the first message is 1970-01-01, therefore the
    # beginning time is 1 s.
    msgs = _trim(trimmer_comp_cls, port_data, {"begin": "00:00:01", "gmt": True})

    # Make sure the seeking operation raised `bt2.TryAgain`
    assert port_data.seek_try_again_count == 0

    # The automatic seeking operation of the library makes the time of
    # the stream beginning message the beginning time.
    assert [_msg_summary(msg) for msg in msgs] == [
        ("stream-beginning", 1),
        ("event", 2),
        ("stream-end", None),
    ]
