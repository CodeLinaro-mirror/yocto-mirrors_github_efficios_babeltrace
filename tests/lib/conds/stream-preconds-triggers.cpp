/*
 * SPDX-License-Identifier: GPL-2.0-only
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/mip.hpp"
#include "cpp-common/bt2/value.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

using FrozenStreamTripFunc = std::function<void(bt_stream *)>;

/*
 * Adds a "frozen stream" trigger built from `tripFunc`.
 *
 * Instantiates a stream, freezes it by creating an event message for
 * it, then calls `tripFunc` on the frozen stream.
 */
void addFrozenStreamTrigger(CondTriggers& triggers, FrozenStreamTripFunc tripFunc,
                            const std::string& condId, const std::uint64_t mipVersion,
                            const std::string_view nameSuffix = {})
{
    triggers.emplace_back(makeRunInMsgIterNextTrigger(
        [tripFunc = std::move(tripFunc)](const auto selfMsgIter, auto& msgs) {
            const auto traceCls = selfMsgIter.component().createTraceClass();
            const auto streamCls = traceCls->createStreamClass();
            const auto stream = streamCls->instantiate(*traceCls->instantiate());

            /* Creating the event message freezes the stream */
            const auto evMsg =
                selfMsgIter.createEventMessage(*streamCls->createEventClass(), *stream);

            tripFunc(stream->libObjPtr());

            /*
             * Should never reach here because the trigger function is
             * expected to abort.
             *
             * Still, append the message so that, if the assertion
             * regresses, the caller doesn't loop forever.
             */
            msgs.append(evMsg->shared());
        },
        CondTrigger::Type::Pre, condId, mipVersion, nameSuffix));
}

} /* namespace */

/*
 * Adds stream API precondition failure triggers.
 */
void addStreamPreCondsTriggers(CondTriggers& triggers)
{
    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_stream_create(nullptr, nullptr);
        },
        "stream-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_create(nullptr, nullptr);
        },
        "stream-create:not-null:stream-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_stream_create_with_id(nullptr, nullptr, 0);
        },
        "stream-create-with-id:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_create_with_id(nullptr, nullptr, 0);
        },
        "stream-create-with-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_borrow_class(nullptr);
        },
        "stream-borrow-class:not-null:stream");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_borrow_trace(nullptr);
        },
        "stream-borrow-trace:not-null:stream");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_get_name(nullptr);
        },
        "stream-get-name:not-null:stream");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_stream_set_name(nullptr, nullptr);
        },
        "stream-set-name:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_set_name(nullptr, "name");
        },
        "stream-set-name:not-null:stream");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_get_id(nullptr);
        },
        "stream-get-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_borrow_user_attributes_const(nullptr);
        },
        "stream-borrow-user-attributes-const:not-null:stream");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_set_user_attributes(nullptr, nullptr);
        },
        "stream-set-user-attributes:not-null:stream");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_create(selfComp.createTraceClass()->createStreamClass()->libObjPtr(),
                             nullptr);
        },
        "stream-create:not-null:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto streamCls = traceCls->createStreamClass();

            streamCls->assignsAutomaticStreamId(false);
            bt_stream_create(streamCls->libObjPtr(), traceCls->instantiate()->libObjPtr());
        },
        "stream-create:stream-class-automatically-assigns-stream-ids", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_create(selfComp.createTraceClass()->createStreamClass()->libObjPtr(),
                             selfComp.createTraceClass()->instantiate()->libObjPtr());
        },
        "stream-create:trace-class-is-stream-class-trace-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_create_with_id(selfComp.createTraceClass()->createStreamClass()->libObjPtr(),
                                     nullptr, 0);
        },
        "stream-create-with-id:not-null:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            /* Default: "assigns automatic stream ID" is true */
            bt_stream_create_with_id(traceCls->createStreamClass()->libObjPtr(),
                                     traceCls->instantiate()->libObjPtr(), 0);
        },
        "stream-create-with-id:stream-class-does-not-automatically-assigns-stream-ids", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = selfComp.createTraceClass()->createStreamClass();

            streamCls->assignsAutomaticStreamId(false);
            bt_stream_create_with_id(streamCls->libObjPtr(),
                                     selfComp.createTraceClass()->instantiate()->libObjPtr(), 0);
        },
        "stream-create-with-id:trace-class-is-stream-class-trace-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto streamCls = traceCls->createStreamClass();

            streamCls->assignsAutomaticStreamId(false);

            const auto trace = traceCls->instantiate();

            /* First stream: ID 0 (succeeds) */
            streamCls->instantiate(*trace, 0);

            /* Second stream: same id (must trip `stream-id-is-unique`) */
            bt_stream_create_with_id(streamCls->libObjPtr(), trace->libObjPtr(), 0);
        },
        "stream-create-with-id:stream-id-is-unique", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_stream_set_name(
                traceCls->createStreamClass()->instantiate(*traceCls->instantiate())->libObjPtr(),
                nullptr);
        },
        "stream-set-name:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_stream_set_user_attributes(
                traceCls->createStreamClass()->instantiate(*traceCls->instantiate())->libObjPtr(),
                nullptr);
        },
        "stream-set-user-attributes:not-null:user-attributes-value-object", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_stream_set_user_attributes(
                traceCls->createStreamClass()->instantiate(*traceCls->instantiate())->libObjPtr(),
                bt2::BoolValue::create()->libObjPtr());
        },
        "stream-set-user-attributes:is-map-value:user-attributes", 0);

    for (std::uint64_t mipVersion = 0; mipVersion <= bt2::getMaximalMipVersion(); ++mipVersion) {
        const auto suffix = fmt::format("mip{}", mipVersion);

        addFrozenStreamTrigger(
            triggers,
            [](const auto libStreamPtr) {
                bt_stream_set_name(libStreamPtr, "new-name");
            },
            "stream-set-name:not-frozen:stream", mipVersion, suffix);

        addFrozenStreamTrigger(
            triggers,
            [](const auto libStreamPtr) {
                bt_stream_set_user_attributes(libStreamPtr, bt2::MapValue::create()->libObjPtr());
            },
            "stream-set-user-attributes:not-frozen:stream", mipVersion, suffix);
    }
}
