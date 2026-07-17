# SPDX-FileCopyrightText: 2019 Simon Marchi <simon.marchi@efficios.com>
# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

# Test how log level options are applied to the `babeltrace.support-info`
# queries made as part of the automatic source discovery process, by the
# convert command.

import pytest
import bt_tests_cli_utils as btu_cli


@pytest.mark.parametrize(
    ["cli_args", "expected"],
    [
        # The default.
        pytest.param(
            ["non-opt-1", "non-opt-2"],
            [
                "non-opt-1 LoggingLevel.WARNING",
                "non-opt-2 LoggingLevel.WARNING",
            ],
            id="default",
        ),
        # Specific log levels passed to non-options.
        pytest.param(
            [
                "non-opt-1",
                "-l",
                "INFO",
                "non-opt-2",
                "-l",
                "DEBUG",
                "non-opt-3",
                "-l",
                "ERROR",
                "non-opt-4",
            ],
            [
                "non-opt-1 LoggingLevel.INFO",
                "non-opt-2 LoggingLevel.DEBUG",
                "non-opt-3 LoggingLevel.ERROR",
                "non-opt-4 LoggingLevel.WARNING",
            ],
            id="specific",
        ),
        # Interaction between a top-level log level and specific log levels.
        pytest.param(
            [
                "-l",
                "INFO",
                "non-opt-1",
                "-l",
                "ERROR",
                "non-opt-2",
                "non-opt-3",
                "-l",
                "TRACE",
            ],
            [
                "non-opt-1 LoggingLevel.ERROR",
                "non-opt-2 LoggingLevel.INFO",
                "non-opt-3 LoggingLevel.TRACE",
            ],
            id="top-level",
        ),
    ],
)
def test_query_log_level(build_root_dir, common_data_dir, cli_args, expected):
    plugin_dir = common_data_dir / "auto-src-comp-discovery/query-log-level"
    result = btu_cli.run_cli(
        build_root_dir,
        cli_args,
        plugin_paths=[plugin_dir],
        check=True,
    )

    # The component's `babeltrace.support-info` query prints the input name and
    # the effective log level on stdout, in command-line order.
    assert [line for line in result.stdout.splitlines() if line.strip()] == expected
