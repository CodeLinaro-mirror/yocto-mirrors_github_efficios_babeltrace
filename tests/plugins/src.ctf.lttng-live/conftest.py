# SPDX-FileCopyrightText: 2025 Philippe Proulx <pproulx@efficios.com>
# SPDX-License-Identifier: MIT

# pyright: strict, reportMissingTypeStubs=false, reportPrivateUsage=false

import typing

import bt2
import pytest
import bt_tests_utils as btu
from lttng_live_server import LttngLiveServerProcess


@pytest.fixture(scope="module")
def live_comp_cls() -> bt2._SourceComponentClassConst:
    return btu.plugin_by_name(
        "ctf"
    ).source_component_classes[  # pyright: ignore[reportOptionalMemberAccess]
        "lttng-live"
    ]


@pytest.fixture(scope="module")
def test_data_dir():
    return btu.this_src_dir(__file__)


# Factory fixture: call it like
# LttngLiveServerProcess.from_config_file() to create and start a faux
# LTTng live server process.
#
# A server which you create this way is waited for (up to 10 seconds)
# and closed when the test ends.
@pytest.fixture
def start_lttng_live_server() -> (
    typing.Iterator[typing.Callable[..., LttngLiveServerProcess]]
):
    server: typing.Optional[LttngLiveServerProcess] = None

    def start(*args: typing.Any, **kwargs: typing.Any) -> LttngLiveServerProcess:
        nonlocal server

        assert server is None, "only one server per test is supported"
        server = LttngLiveServerProcess.from_config_file(*args, **kwargs)
        server.start()
        return server

    yield start

    if server is not None:
        server.wait(timeout=10)

        if server.is_alive:
            # Server might still be alive if the test errored out
            # without cleanly closing the connection: just close it.
            server.close()
