# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import textwrap

import bt_tests_utils as btu
import bt_tests_cli_utils as btu_cli


def test_output(build_root_dir):
    # Directory containing the test plugin provider, which provides the
    # deterministic `test-provider-{1,2,3}` providers.
    provider_dir = build_root_dir / "tests/utils/plugin-providers/.libs"
    so_path = btu.shared_lib_path(provider_dir / "bt-plugin-provider")

    # Restrict plugin provider discovery to the test provider so that
    # the output is deterministic, independent of what else happens to
    # be built.
    cli_run = btu_cli.run_cli(
        build_root_dir,
        ["list-plugin-providers"],
        check=True,
        extra_env={
            "BABELTRACE_PLUGIN_PROVIDER_PATH": str(provider_dir),
        },
    )

    # The command first prints the list of provider search paths, which
    # includes machine-specific home and system directories.
    #
    # Skip it and compare everything from the summary line onward.
    actual = cli_run.stdout[cli_run.stdout.index("Found ") :].strip()

    expected = textwrap.dedent(f"""
        Found 3 plugin providers.

        test-provider-1:
          Path: {so_path}
          Version: 1.2.3dev
          Description: Provider one
          Author: Test Author
          License: MIT

        test-provider-2:
          Path: {so_path}
          Version: 4.5.6
          Description: Provider two
          Author: Another Author
          License: GPLv2

        test-provider-3:
          Path: {so_path}
          Description: (None)
          Author: (Unknown)
          License: (Unknown)
        """).strip()

    assert actual == expected
