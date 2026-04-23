# SPDX-License-Identifier: GPL-2.0-only
# Copyright (C) 2026 EfficiOS Inc.

import pathlib

import bt2
import pytest
import bt_tests_utils as btu


class TestSet:
    @pytest.fixture(scope="class")
    def providers(self):
        return bt2.plugin_providers()

    # The exact set of plugin provider names the test environment loads.
    #
    # The test environment always builds the `so` plugin provider and
    # always loads the test plugin provider, but only builds the
    # `python` plugin provider with `--enable-python-plugins`. It
    # exposes all their directories through
    # `BABELTRACE_PLUGIN_PROVIDER_PATH`, therefore detect whether it
    # built the `python` provider by looking for its shared object.
    @pytest.fixture(scope="class")
    def expected_provider_names(self, build_root_dir):
        names = {"so", "test-provider-1", "test-provider-2", "test-provider-3"}

        if btu.shared_lib_path(
            build_root_dir
            / ("src/python-plugin-provider/.libs/babeltrace2-python-plugin-provider")
        ).exists():
            names.add("python")

        return names

    def test_iter(self, providers, expected_provider_names):
        assert {provider.name for provider in providers} == expected_provider_names

    def test_len(self, providers, expected_provider_names):
        assert len(providers) == len(expected_provider_names)

    def test_getitem(self, providers, expected_provider_names):
        assert providers[0].name in expected_provider_names

    def test_getitem_out_of_range(self, providers):
        with pytest.raises(IndexError):
            providers[len(providers)]


class TestProps:
    @pytest.fixture(scope="class")
    def providers_by_name(self):
        return {provider.name: provider for provider in bt2.plugin_providers()}

    @pytest.fixture(scope="class")
    def provider_1(self, providers_by_name):
        return providers_by_name["test-provider-1"]

    @pytest.fixture(scope="class")
    def provider_2(self, providers_by_name):
        return providers_by_name["test-provider-2"]

    @pytest.fixture(scope="class")
    def provider_3(self, providers_by_name):
        return providers_by_name["test-provider-3"]

    def test_name(self, provider_1):
        assert provider_1.name == "test-provider-1"

    def test_description(self, provider_1):
        assert provider_1.description == "Provider one"

    def test_author(self, provider_1):
        assert provider_1.author == "Test Author"

    def test_license(self, provider_1):
        assert provider_1.license == "MIT"

    def test_path(self, provider_1):
        assert provider_1.path is not None
        assert provider_1.path.endswith(
            str(btu.shared_lib_path(pathlib.Path("bt-plugin-provider")))
        )

    def test_version(self, provider_1):
        version = provider_1.version
        assert version is not None
        assert version.major == 1
        assert version.minor == 2
        assert version.patch == 3
        assert version.extra == "dev"

    def test_version_without_extra(self, provider_2):
        version = provider_2.version
        assert version is not None
        assert version.major == 4
        assert version.minor == 5
        assert version.patch == 6
        assert version.extra is None

    def test_description_none(self, provider_3):
        assert provider_3.description is None

    def test_author_none(self, provider_3):
        assert provider_3.author is None

    def test_license_none(self, provider_3):
        assert provider_3.license is None

    def test_version_none(self, provider_3):
        assert provider_3.version is None
