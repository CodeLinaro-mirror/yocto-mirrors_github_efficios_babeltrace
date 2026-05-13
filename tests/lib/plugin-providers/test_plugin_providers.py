# SPDX-FileCopyrightText: 2026 EfficiOS Inc.
# SPDX-License-Identifier: GPL-2.0-only

import pytest
import bt_tests_utils as btu


@pytest.mark.parametrize(
    "provider_subdir,expected_init,expected_provider,expected_exit",
    [
        # Both providers get initialized, loaded and finalized
        # exactly once.
        pytest.param(
            "multi",
            ["init test-provider-one", "init test-provider-two"],
            ["provider test-provider-one", "provider test-provider-two"],
            ["exit test-provider-one", "exit test-provider-two"],
            id="init-and-exit",
        ),
        # A provider whose initialization function fails is not loaded
        # and does not have its finalization function called.
        pytest.param(
            "init-fail",
            ["init test-provider-init-fail", "init test-provider-init-ok"],
            ["provider test-provider-init-ok"],
            ["exit test-provider-init-ok"],
            id="init-fail",
        ),
        # When two providers share the same name, the duplicate is
        # skipped entirely: its initialization function is not called,
        # it's not loaded, and its finalization function is not called.
        pytest.param(
            "dup",
            ["init test-provider-dup"],
            ["provider test-provider-dup"],
            ["exit test-provider-dup"],
            id="duplicate-name",
        ),
    ],
)
def test_plugin_provider(
    build_root_dir, provider_subdir, expected_init, expected_provider, expected_exit
):
    build_dir = btu.build_dir_of_source_file(build_root_dir, __file__)
    provider_dir = build_dir / provider_subdir / ".libs"

    result = btu.run(
        build_root_dir,
        build_dir / "load-plugin-providers.bin",
        [],
        extra_env={
            # Discover plugin providers only from our test directory.
            "BABELTRACE_PLUGIN_PROVIDER_PATH": str(provider_dir),
        },
    )

    assert result.returncode == 0

    lines = result.stdout.splitlines()

    # With `--enable-built-in-python-plugin-support`, the Python plugin
    # provider is static, so will show up here. Remove it.
    if "provider python" in lines:
        lines.remove("provider python")

    init_lines = [line for line in lines if line.startswith("init ")]
    provider_lines = [line for line in lines if line.startswith("provider ")]
    exit_lines = [line for line in lines if line.startswith("exit ")]

    assert sorted(init_lines) == expected_init
    assert sorted(provider_lines) == expected_provider
    assert sorted(exit_lines) == expected_exit

    # All initializations happen, then the providers are loaded, then
    # all finalizations happen.
    assert lines == init_lines + provider_lines + exit_lines
