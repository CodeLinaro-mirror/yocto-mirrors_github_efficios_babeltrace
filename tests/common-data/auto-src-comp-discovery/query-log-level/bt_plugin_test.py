# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 EfficiOS Inc.
#

import bt2

# This file defines a source component class to help verify the effective log
# level during queries. More specifically, the `babeltrace.support-info`
# queries made during the automatic source discovery process.
#
# The query prints the input name and the effective log level on stdout. The
# tests capture that output: the CLI test captures the CLI's stdout, while the
# Python test captures its own, through pytest's `capsys` fixture.


@bt2.plugin_component_class
class TestSource(
    bt2._UserSourceComponent, message_iterator_class=bt2._UserMessageIterator
):
    @staticmethod
    def _user_query(priv_query_exec, obj, params, method_obj):
        if obj == "babeltrace.support-info":
            if params["type"] != "string":
                return 0

            print("{} {}".format(params["input"], priv_query_exec.logging_level))
            return 1
        else:
            raise bt2.UnknownObject


bt2.register_plugin(module_name=__name__, name="test")
