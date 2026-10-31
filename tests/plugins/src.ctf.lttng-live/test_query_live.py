# SPDX-FileCopyrightText: 2025 EfficiOS, Inc.
# SPDX-License-Identifier: GPL-2.0-only

import bt2
import pytest
import bt_tests_utils as btu


@pytest.mark.parametrize(
    "query_name",
    [
        "sessions",
        "babeltrace.support-info",
    ],
)
def test_no_params(live_comp_cls, query_name):
    with btu.raises_bt2_error_with_causes(
        "top-level is not a map value", first_only=True
    ):
        bt2.QueryExecutor(live_comp_cls, query_name).query()


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

    with btu.raises_bt2_error_with_causes(
        bad_v215_session_list_reply_case.expected_msg
    ):
        try:
            bt2.QueryExecutor(
                live_comp_cls, "sessions", {"url": server.base_url}
            ).query()
        finally:
            # Close server now to avoid waiting for the timeout since
            # the client didn't disconnect cleanly.
            server.close()
