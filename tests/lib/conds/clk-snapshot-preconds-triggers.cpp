/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <cstdint>
#include <functional>
#include <utility>

#include <babeltrace2/babeltrace.h>

#include "common/assert.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

using ClkSnapshotTripFunc = std::function<void(const bt_clock_snapshot *)>;

/*
 * Adds a "clock snapshot" trigger built from `tripFunc`.
 *
 * Builds a trace class with a stream class having a default clock class
 * so that the resulting stream beginning message carries a default
 * clock snapshot, then calls `tripFunc` on that clock snapshot.
 */
void addClkSnapshotTrigger(CondTriggers& triggers, ClkSnapshotTripFunc tripFunc,
                           const std::string& condId)
{
    triggers.emplace_back(makeRunInMsgIterNextTrigger(
        [tripFunc = std::move(tripFunc)](const auto selfMsgIter, auto& msgs) {
            const auto traceCls = selfMsgIter.component().createTraceClass();
            const auto streamCls = traceCls->createStreamClass();

            streamCls->defaultClockClass(*selfMsgIter.component().createClockClass());

            const auto streamBegMsg = selfMsgIter.createStreamBeginningMessage(
                *streamCls->instantiate(*traceCls->instantiate()));

            streamBegMsg->defaultClockSnapshot(123);

            const auto cs = streamBegMsg->defaultClockSnapshot();

            BT_ASSERT(cs);
            tripFunc(cs->libObjPtr());

            /*
             * Should never reach here because the trip function is
             * expected to abort. Still, append the message so that, if
             * the assertion regresses, the caller doesn't loop forever.
             */
            msgs.append(streamBegMsg->shared());
        },
        CondTrigger::Type::Pre, condId, 0));
}

} /* namespace */

/*
 * Adds clock snapshot API precondition failure triggers.
 */
void addClkSnapshotPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_clock_snapshot_get_value(nullptr);
        },
        "clock-snapshot-get-value:not-null:clock-snapshot");

    addPreTrigger(
        triggers,
        [] {
            std::int64_t ns;

            bt_clock_snapshot_get_ns_from_origin(nullptr, &ns);
        },
        "clock-snapshot-get-ns-from-origin:not-null:clock-snapshot");

    addClkSnapshotTrigger(
        triggers,
        [](const auto libClkSnapshotPtr) {
            withCurrentThreadError([&] {
                std::int64_t ns;

                bt_clock_snapshot_get_ns_from_origin(libClkSnapshotPtr, &ns);
            });
        },
        "clock-snapshot-get-ns-from-origin:no-error");

    addClkSnapshotTrigger(
        triggers,
        [](const auto libClkSnapshotPtr) {
            bt_clock_snapshot_get_ns_from_origin(libClkSnapshotPtr, nullptr);
        },
        "clock-snapshot-get-ns-from-origin:not-null:value-ns-output");

    addPreTrigger(
        triggers,
        [] {
            bt_clock_snapshot_borrow_clock_class_const(nullptr);
        },
        "clock-snapshot-borrow-clock-class-const:not-null:clock-snapshot");
}
