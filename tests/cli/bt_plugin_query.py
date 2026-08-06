# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 EfficiOS Inc.
#

import bt2
import bt_tests_cli_utils as btu_cli


@bt2.plugin_component_class
class SourceWithQueryThatPrintsParams(
    bt2._UserSourceComponent, message_iterator_class=bt2._UserMessageIterator
):
    @classmethod
    def _user_query(cls, executor, obj, params, method_obj):
        if obj == "please-fail":
            raise ValueError("catastrophic failure")

        return f"{obj}:{btu_cli.cli_params_from_obj(params)}"


bt2.register_plugin(__name__, "query")
