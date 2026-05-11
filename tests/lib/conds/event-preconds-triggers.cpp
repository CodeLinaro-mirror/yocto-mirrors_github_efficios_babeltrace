/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Adds event API precondition failure triggers.
 */
void addEventPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_event_borrow_class(nullptr);
        },
        "event-borrow-class:not-null:event");

    addPreTrigger(
        triggers,
        [] {
            bt_event_borrow_stream(nullptr);
        },
        "event-borrow-stream:not-null:event");

    addPreTrigger(
        triggers,
        [] {
            bt_event_borrow_common_context_field(nullptr);
        },
        "event-borrow-common-context-field:not-null:event");

    addPreTrigger(
        triggers,
        [] {
            bt_event_borrow_specific_context_field(nullptr);
        },
        "event-borrow-specific-context-field:not-null:event");

    addPreTrigger(
        triggers,
        [] {
            bt_event_borrow_payload_field(nullptr);
        },
        "event-borrow-payload-field:not-null:event");

    addPreTrigger(
        triggers,
        [] {
            bt_event_borrow_packet(nullptr);
        },
        "event-borrow-packet:not-null:event");
}
