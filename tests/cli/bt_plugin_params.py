# SPDX-License-Identifier: GPL-2.0-only
#
# Copyright (C) 2019 EfficiOS Inc.
#

import bt2
import bt_tests_cli_utils as btu_cli


@bt2.plugin_component_class
class SinkThatPrintsParams(bt2._UserSinkComponent):
    def __init__(self, config, params, obj):
        self._add_input_port("in")
        print(btu_cli.cli_params_from_obj(params))

    def _user_consume(self):
        raise bt2.Stop


bt2.register_plugin(__name__, "params")
