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


# A case for the validation of a session within a v2.15 protocol session
# list reply: the server options making it announce a bogus session
# (zero or oversized hostname/name length), paired with the error
# message substring the client is expected to produce when rejecting it.
class _BadV215SessionListReplyCase(typing.NamedTuple):
    server_options: typing.Dict[str, int]
    expected_msg: str


@pytest.fixture(
    params=[
        pytest.param(
            _BadV215SessionListReplyCase(
                {"override_hostname_len": 0},
                "Session hostname length is zero",
            ),
            id="hostname-zero",
        ),
        pytest.param(
            _BadV215SessionListReplyCase(
                {"override_hostname_len": 64 * 1024 + 1},
                "Session hostname length is greater than arbitrary max",
            ),
            id="hostname-too-long",
        ),
        pytest.param(
            _BadV215SessionListReplyCase(
                {"override_session_name_len": 0},
                "Session name length is zero",
            ),
            id="session-name-zero",
        ),
        pytest.param(
            _BadV215SessionListReplyCase(
                {"override_session_name_len": 64 * 1024 + 1},
                "Session name length is greater than arbitrary max",
            ),
            id="session-name-too-long",
        ),
    ]
)
def bad_v215_session_list_reply_case(
    request: pytest.FixtureRequest,
) -> _BadV215SessionListReplyCase:
    return request.param


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
