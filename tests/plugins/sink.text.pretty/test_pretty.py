# SPDX-FileCopyrightText: 2020 EfficiOS, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import bt2
import pytest
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
