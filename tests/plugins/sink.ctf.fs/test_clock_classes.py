# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import enum
import uuid

import bt2
import pytest
import bt_tests_utils as btu


# How the default clock classes of the two stream classes of `_Src`
# below relate to each other.
class _Mode(enum.Enum):
    # Both stream classes share the very same default clock class.
    SHARED = "shared"

    # Each stream class has its own default clock class, but both have
    # the same identity and the same other properties.
    DISTINCT = "distinct"


class _Iter(bt2._UserMessageIterator):
    def __init__(self, config, port):
        tc, stream_clss, event_clss = port.user_data
        trace = tc()
        self._msgs = []

        for i, (sc, ec) in enumerate(zip(stream_clss, event_clss)):
            stream = trace.create_stream(sc, name="stream-{}".format(i))
            self._msgs += [
                self._create_stream_beginning_message(stream),
                self._create_event_message(ec, stream, default_clock_snapshot=100 + i),
                self._create_stream_end_message(stream),
            ]

    def __next__(self):
        if not self._msgs:
            raise StopIteration

        return self._msgs.pop(0)


# Test source component class.
#
# The trace class has two stream classes, each one having its own
# default clock class, and the message iterator emits one event for
# each one.
#
# The initialization object (`obj`) is a `_Mode` member controlling how
# the two default clock classes relate to each other.
class _Src(bt2._UserSourceComponent, message_iterator_class=_Iter):
    def __init__(self, config, params, obj):
        tc = self._create_trace_class()

        def create_clock_class():
            # Give the clock classes an identity so that they may be
            # correlated even though they're distinct objects.
            if self._graph_mip_version == 0:
                ident = {"uuid": uuid.UUID("d1a4e2b3-0c5f-4a67-9b8d-2e3f4a5b6c7d")}
            else:
                ident = {"uid": "the-uid"}

            return self._create_clock_class(
                frequency=1000000000,
                name="the-clock",
                **ident,
            )

        clk_cls_1 = create_clock_class()

        if obj == _Mode.SHARED:
            clk_cls_2 = clk_cls_1
        else:
            assert obj == _Mode.DISTINCT
            clk_cls_2 = create_clock_class()

        stream_clss = [
            tc.create_stream_class(default_clock_class=clk_cls)
            for clk_cls in (clk_cls_1, clk_cls_2)
        ]
        event_clss = [sc.create_event_class(name="the-event") for sc in stream_clss]
        self._add_output_port("out", user_data=(tc, stream_clss, event_clss))

    @staticmethod
    def _user_get_supported_mip_versions(params, obj, log_level):
        return [0, 1]


# Makes a `sink.ctf.fs` component write the trace of a `_Src` component
# using the mode `mode` as CTF `ctf_version`, and returns the resulting
# trace directory.
def _write_trace(sink_ctf_comp_cls, tmp_path_factory, mode, ctf_version):
    trace_dir = (
        tmp_path_factory.mktemp(f"clock-classes-{mode.value}-{ctf_version}")
        / "the-trace"
    )

    btu.convert(
        bt2.ComponentSpec(_Src, obj=mode),
        btu.SinkComponentSpec(
            sink_ctf_comp_cls,
            {
                "path": str(trace_dir),
                "assume-single-trace": True,
                "quiet": True,
                "ctf-version": ctf_version,
            },
        ),
    )

    return trace_dir


_CTF_VERSIONS = [pytest.param(v, id=f"ctf-{v}") for v in ("1.8", "2")]
_MODES = [pytest.param(mode, id=mode.value) for mode in _Mode]


# The component writes exactly one clock class definition per distinct
# clock class of the trace, whatever the number of stream classes
# using it.
@pytest.mark.parametrize("mode", _MODES)
@pytest.mark.parametrize("ctf_version", _CTF_VERSIONS)
def test_clk_cls_count(sink_ctf_comp_cls, tmp_path_factory, ctf_version, mode):
    trace_dir = _write_trace(sink_ctf_comp_cls, tmp_path_factory, mode, ctf_version)
    assert (trace_dir / "metadata").read_bytes().count(
        b"clock {" if ctf_version == "1.8" else b'"type":"clock-class"'
    ) == (1 if mode == _Mode.SHARED else 2)


# The resulting trace is readable.
@pytest.mark.parametrize("mode", _MODES)
@pytest.mark.parametrize("ctf_version", _CTF_VERSIONS)
def test_read_back(sink_ctf_comp_cls, tmp_path_factory, ctf_version, mode):
    trace_dir = _write_trace(sink_ctf_comp_cls, tmp_path_factory, mode, ctf_version)
    events = btu.tcmi_events(str(trace_dir))
    assert len(events) == 2
    assert all(e.name == "the-event" for e in events)
