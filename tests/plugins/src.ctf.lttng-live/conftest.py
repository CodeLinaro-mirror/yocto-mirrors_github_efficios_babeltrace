# SPDX-FileCopyrightText: 2025 Philippe Proulx <pproulx@efficios.com>
# SPDX-License-Identifier: MIT

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import bt2
import pytest
import bt_tests_utils as btu


@pytest.fixture(scope="module")
def live_comp_cls() -> bt2._SourceComponentClassConst:
    return btu.plugin_by_name(
        "ctf"
    ).source_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
        "lttng-live"
    ]
