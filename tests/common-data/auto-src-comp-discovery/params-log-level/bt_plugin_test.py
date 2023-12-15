# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 EfficiOS Inc.
#

import os

import bt2
import bt_tests_cli_utils as btu_cli

# This file defines source component classes to help verify the parameters, log
# levels, and Python objects passed to components. The `what` parameter defines
# what to report (see below).
#
# Whatever we report, we print it on stdout, prefixed with our class name. The
# tests capture that output: the CLI tests capture the CLI's stdout, while the
# Python tests capture their own, through pytest's `capsys` fixture.


class Base:
    def __init__(self, params, obj):
        comp_cls_name = self.__class__.__name__

        if params["what"] == "test-params":
            # Report the received parameters, filtering out those that don't
            # start with `test-`.
            what = btu_cli.cli_params_from_obj(
                {k: v for k, v in params.items() if k.startswith("test-")}
            )
        elif params["what"] == "log-level":
            # Report the received log level.
            what = self.logging_level
        elif params["what"] == "python-obj":
            # Report the identity of the Python object we received, so that a
            # test can check that we received the exact object it passed.
            what = id(obj) if obj is not None else None
        else:
            assert False

        print(f"{comp_cls_name}: {what}")


@bt2.plugin_component_class
class TestSourceA(
    Base, bt2._UserSourceComponent, message_iterator_class=bt2._UserMessageIterator
):
    def __init__(self, config, params, obj):
        super().__init__(params, obj)

    @staticmethod
    def _user_query(priv_query_exec, obj, params, method_obj):
        # Match files starting with 'aaa'.

        if obj == "babeltrace.support-info":
            if params["type"] != "file":
                return 0

            name = os.path.basename(str(params["input"]))

            if name.startswith("aaa"):
                return {"weight": 1, "group": "aaa"}
            else:
                return 0
        else:
            raise bt2.UnknownObject


@bt2.plugin_component_class
class TestSourceB(
    Base, bt2._UserSourceComponent, message_iterator_class=bt2._UserMessageIterator
):
    def __init__(self, config, params, obj):
        super().__init__(params, obj)

    @staticmethod
    def _user_query(priv_query_exec, obj, params, method_obj):
        # Match files starting with 'bbb'.

        if obj == "babeltrace.support-info":
            if params["type"] != "file":
                return 0

            name = os.path.basename(str(params["input"]))

            if name.startswith("bbb"):
                return {"weight": 1, "group": "bbb"}
            else:
                return 0
        else:
            raise bt2.UnknownObject


bt2.register_plugin(module_name=__name__, name="test")
