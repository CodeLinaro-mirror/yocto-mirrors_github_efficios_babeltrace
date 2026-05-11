/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/self-component-port.hpp"
#include "cpp-common/bt2/trace-ir.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Add triggers for packet API.
 */
void addPktPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_packet_borrow_stream(nullptr);
        },
        "packet-borrow-stream:not-null:packet");

    addPreTrigger(
        triggers,
        [] {
            bt_packet_borrow_context_field(nullptr);
        },
        "packet-borrow-context-field:not-null:packet");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_packet_create(nullptr);
        },
        "packet-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_packet_create(nullptr);
        },
        "packet-create:not-null:stream");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            /*
             * The stream class doesn't support packets: instantiating a
             * stream from it and then asking for a packet must trip the
             * "stream class supports packets" precondition.
             */
            traceCls->createStreamClass()->instantiate(*traceCls->instantiate())->createPacket();
        },
        CondTrigger::Type::Pre, "packet-create:stream-class-supports-packets", 0));
}
