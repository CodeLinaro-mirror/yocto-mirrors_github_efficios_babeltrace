# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2026 EfficiOS Inc.
#

import bt2


class TestIter(bt2._UserMessageIterator):
    def __init__(self, config, output_port):
        sc = output_port.user_data["sc"]
        tc = sc.trace_class
        s = tc().create_stream(sc)

        self._msgs = [
            self._create_stream_beginning_message(s),
            self._create_stream_end_message(s),
        ]

    def __next__(self):
        if len(self._msgs) == 0:
            raise StopIteration

        return self._msgs.pop(0)


@bt2.plugin_component_class
class TestSource(bt2._UserSourceComponent, message_iterator_class=TestIter):
    """
    A source component that recognizes the arbitrary string
    input `radis`.
    """

    def __init__(self, config, params, obj):
        self._add_output_port(
            "out", {"sc": self._create_trace_class().create_stream_class()}
        )

    @staticmethod
    def _user_query(priv_query_exec, obj, params, method_obj):
        if obj == "babeltrace.support-info":
            return (
                1.0
                if params["type"] == "string" and params["input"] == "radis"
                else 0.0
            )
        else:
            raise bt2.UnknownObject


bt2.register_plugin(module_name=__name__, name="test")
