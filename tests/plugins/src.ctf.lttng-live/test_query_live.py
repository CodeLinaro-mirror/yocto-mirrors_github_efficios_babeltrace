# SPDX-FileCopyrightText: 2025 EfficiOS, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import bt2
import pytest


@pytest.mark.parametrize(
    "query_name",
    [
        "sessions",
        "babeltrace.support-info",
    ],
)
def test_no_params(live_comp_cls, query_name):
    with pytest.raises(bt2._Error) as exc_info:
        bt2.QueryExecutor(live_comp_cls, query_name).query()

    assert "top-level is not a map value" in exc_info.value[0].message


# Query the session list from a relay that returns a bad session list.
# See `bad_v215_session_list_reply_case()` for the various cases.
def test_list_sessions_bad_reply(
    live_comp_cls,
    ctf_traces_dir,
    test_data_dir,
    start_lttng_live_server,
    bad_v215_session_list_reply_case,
):
    server = start_lttng_live_server(
        str(test_data_dir / "base.json"),
        trace_path_prefix=str(ctf_traces_dir),
        max_minor_version=15,
        **bad_v215_session_list_reply_case.server_options,
    )

    with pytest.raises(bt2._Error) as exc_info:
        bt2.QueryExecutor(live_comp_cls, "sessions", {"url": server.base_url}).query()

    # Close server now to avoid waiting for the timeout since the
    # client didn't disconnect cleanly.
    server.close()

    assert bad_v215_session_list_reply_case.expected_msg in str(exc_info.value)
