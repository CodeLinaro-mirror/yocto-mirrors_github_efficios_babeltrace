# SPDX-FileCopyrightText: 2019 Simon Marchi <simon.marchi@efficios.com>
# SPDX-FileCopyrightText: 2025 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import textwrap

import pytest
import bt_tests_cli_utils as btu_cli


@pytest.fixture(scope="module")
def plugin_dir(common_data_dir):
    return common_data_dir / "auto-src-comp-discovery/params-log-level"


@pytest.mark.parametrize(
    ["cli_args", "expected"],
    [
        # Apply parameters to two components from one non-option argument.
        pytest.param(
            lambda plugin_dir: [
                str(plugin_dir / "dir-ab"),
                btu_cli.CliParams({"what": "test-params", "test-allo": "madame"}),
            ],
            """
            TestSourceA: test-allo="madame"
            TestSourceB: test-allo="madame"
            """,
            id="two-components-from-one-non-option-arg",
        ),
        # Apply parameters to two components from two distinct
        # non-option arguments.
        pytest.param(
            lambda plugin_dir: [
                str(plugin_dir / "dir-a"),
                btu_cli.CliParams({"test-allo": "madame"}),
                btu_cli.CliParams({"what": "test-params"}),
                str(plugin_dir / "dir-b"),
                btu_cli.CliParams({"test-bonjour": "monsieur"}),
                btu_cli.CliParams({"what": "test-params"}),
            ],
            """
            TestSourceA: test-allo="madame"
            TestSourceB: test-bonjour="monsieur"
            """,
            id="two-non-option-args",
        ),
        # Apply parameters to one component coming from one non-option argument
        # and one component coming from two non-option arguments (1).
        pytest.param(
            lambda plugin_dir: [
                str(plugin_dir / "dir-a"),
                btu_cli.CliParams({"what": "test-params", "test-allo": "madame"}),
                str(plugin_dir / "dir-ab"),
                btu_cli.CliParams({"what": "test-params", "test-bonjour": "monsieur"}),
            ],
            """
            TestSourceA: test-allo="madame", test-bonjour="monsieur"
            TestSourceB: test-bonjour="monsieur"
            """,
            id="one-and-two-non-option-args-1",
        ),
        # Apply parameters to one component coming from one non-option argument
        # and one component coming from two non-option arguments (2).
        pytest.param(
            lambda plugin_dir: [
                str(plugin_dir / "dir-ab"),
                btu_cli.CliParams(
                    {
                        "what": "test-params",
                        "test-bonjour": "madame",
                        "test-salut": "les amis",
                    }
                ),
                str(plugin_dir / "dir-a"),
                btu_cli.CliParams({"what": "test-params", "test-bonjour": "monsieur"}),
            ],
            """
            TestSourceA: test-bonjour="monsieur", test-salut="les amis"
            TestSourceB: test-bonjour="madame", test-salut="les amis"
            """,
            id="one-and-two-non-option-args-2",
        ),
    ],
)
def test_params(build_root_dir, plugin_dir, cli_args, expected):
    result = btu_cli.run_cli(
        build_root_dir,
        ["convert"] + cli_args(plugin_dir),
        plugin_paths=[plugin_dir],
        check=True,
    )

    # The instantiated components print their received parameters on stdout.
    # The order in which the lines are printed is unpredictable (it depends
    # on the order the filesystem is walked), so sort before comparing.
    actual = sorted(line for line in result.stdout.splitlines() if line.strip())
    expected = sorted(
        line.strip() for line in textwrap.dedent(expected).splitlines() if line.strip()
    )
    assert actual == expected
