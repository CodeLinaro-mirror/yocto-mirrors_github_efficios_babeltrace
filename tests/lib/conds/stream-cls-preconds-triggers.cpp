/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/mip.hpp"
#include "cpp-common/bt2/value.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

/*
 * Creates a basic stream class on `selfComp` for use as the "valid" subject
 * of a precondition violation trigger.
 *
 * The created stream class supports packets and has a default clock
 * class so that most subsequent setters succeed and the test can target
 * a single precondition.
 */
bt2::StreamClass::Shared createBasicStreamCls(const bt2::SelfComponent selfComp)
{
    const auto traceCls = selfComp.createTraceClass();

    traceCls->assignsAutomaticStreamClassId(true);

    const auto streamCls = traceCls->createStreamClass();

    streamCls->supportsPackets(true, false, false);
    streamCls->defaultClockClass(*selfComp.createClockClass());
    return streamCls;
}

/*
 * Returns a frozen stream class.
 *
 * A stream class is frozen when you create an event class from it.
 */
bt2::StreamClass::Shared createFrozenStreamCls(const bt2::SelfComponent selfComp)
{
    const auto streamCls = createBasicStreamCls(selfComp);

    /* Creating an event class freezes the stream class */
    streamCls->createEventClass();
    return streamCls;
}

} /* namespace */

/*
 * Adds stream class API precondition failure triggers.
 */
void addStreamClsPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_create(nullptr);
        },
        "stream-class-create:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_create_with_id(nullptr, 0);
        },
        "stream-class-create-with-id:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_trace_class(nullptr);
        },
        "stream-class-borrow-trace-class:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_get_namespace(nullptr);
        },
        "stream-class-get-namespace:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_namespace(nullptr, "ns");
        },
        "stream-class-set-namespace:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_get_name(nullptr);
        },
        "stream-class-get-name:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_name(nullptr, "name");
        },
        "stream-class-set-name:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_get_uid(nullptr);
        },
        "stream-class-get-uid:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_uid(nullptr, "uid");
        },
        "stream-class-set-uid:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_get_id(nullptr);
        },
        "stream-class-get-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_get_event_class_count(nullptr);
        },
        "stream-class-get-event-class-count:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_event_class_by_index(nullptr, 0);
        },
        "stream-class-borrow-event-class-by-index:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_event_class_by_id(nullptr, 0);
        },
        "stream-class-borrow-event-class-by-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_packet_context_field_class(nullptr);
        },
        "stream-class-borrow-packet-context-field-class:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_event_common_context_field_class(nullptr);
        },
        "stream-class-borrow-event-common-context-field-class:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_default_clock_class(nullptr, nullptr);
        },
        "stream-class-set-default-clock-class:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_default_clock_class(nullptr);
        },
        "stream-class-borrow-default-clock-class:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_default_clock_class_const(nullptr);
        },
        "stream-class-borrow-default-clock-class-const:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_assigns_automatic_event_class_id(nullptr);
        },
        "stream-class-assigns-automatic-event-class-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_assigns_automatic_event_class_id(nullptr, BT_TRUE);
        },
        "stream-class-set-assigns-automatic-event-class-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_assigns_automatic_stream_id(nullptr);
        },
        "stream-class-assigns-automatic-stream-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_assigns_automatic_stream_id(nullptr, BT_TRUE);
        },
        "stream-class-set-assigns-automatic-stream-id:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_supports_discarded_events(nullptr, BT_TRUE, BT_FALSE);
        },
        "stream-class-set-supports-discarded-events:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_supports_discarded_events(nullptr);
        },
        "stream-class-supports-discarded-events:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_discarded_events_have_default_clock_snapshots(nullptr);
        },
        "stream-class-discarded-events-have-default-clock-snapshots:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_supports_discarded_packets(nullptr, BT_TRUE, BT_FALSE);
        },
        "stream-class-set-supports-discarded-packets:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_supports_discarded_packets(nullptr);
        },
        "stream-class-supports-discarded-packets:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_discarded_packets_have_default_clock_snapshots(nullptr);
        },
        "stream-class-discarded-packets-have-default-clock-snapshots:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_supports_packets(nullptr, BT_TRUE, BT_FALSE, BT_FALSE);
        },
        "stream-class-set-supports-packets:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_supports_packets(nullptr);
        },
        "stream-class-supports-packets:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_packets_have_beginning_default_clock_snapshot(nullptr);
        },
        "stream-class-packets-have-beginning-default-clock-snapshot:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_packets_have_end_default_clock_snapshot(nullptr);
        },
        "stream-class-packets-have-end-default-clock-snapshot:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_borrow_user_attributes_const(nullptr);
        },
        "stream-class-borrow-user-attributes-const:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_user_attributes(nullptr, nullptr);
        },
        "stream-class-set-user-attributes:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_packet_context_field_class(nullptr, nullptr);
        },
        "stream-class-set-packet-context-field-class:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_stream_class_set_event_common_context_field_class(nullptr, nullptr);
        },
        "stream-class-set-event-common-context-field-class:not-null:stream-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->assignsAutomaticStreamClassId(false);
            bt_stream_class_create(traceCls->libObjPtr());
        },
        "stream-class-create:trace-class-automatically-assigns-stream-class-ids", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->assignsAutomaticStreamClassId(true);
            bt_stream_class_create_with_id(traceCls->libObjPtr(), 0);
        },
        "stream-class-create-with-id:trace-class-does-not-automatically-assigns-stream-class-ids",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_name(createBasicStreamCls(selfComp)->libObjPtr(), nullptr);
        },
        "stream-class-set-name:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_name(createFrozenStreamCls(selfComp)->libObjPtr(), "name");
        },
        "stream-class-set-name:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_borrow_event_class_by_index(createBasicStreamCls(selfComp)->libObjPtr(),
                                                        0);
        },
        "stream-class-borrow-event-class-by-index:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_default_clock_class(
                selfComp.createTraceClass()->createStreamClass()->libObjPtr(), nullptr);
        },
        "stream-class-set-default-clock-class:not-null:clock-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_default_clock_class(createFrozenStreamCls(selfComp)->libObjPtr(),
                                                    selfComp.createClockClass()->libObjPtr());
        },
        "stream-class-set-default-clock-class:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_assigns_automatic_event_class_id(
                createFrozenStreamCls(selfComp)->libObjPtr(), BT_TRUE);
        },
        "stream-class-set-assigns-automatic-event-class-id:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_assigns_automatic_stream_id(
                createFrozenStreamCls(selfComp)->libObjPtr(), BT_TRUE);
        },
        "stream-class-set-assigns-automatic-stream-id:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_discarded_events(
                createFrozenStreamCls(selfComp)->libObjPtr(), BT_TRUE, BT_FALSE);
        },
        "stream-class-set-supports-discarded-events:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_discarded_events(
                createBasicStreamCls(selfComp)->libObjPtr(), BT_FALSE, BT_TRUE);
        },
        "stream-class-set-supports-discarded-events:supports-discarded-events-for-default-clock-snapshots",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_discarded_events(
                selfComp.createTraceClass()->createStreamClass()->libObjPtr(), BT_TRUE, BT_TRUE);
        },
        "stream-class-set-supports-discarded-events:has-default-clock-class-for-default-clock-snapshots",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_discarded_packets(
                createFrozenStreamCls(selfComp)->libObjPtr(), BT_TRUE, BT_FALSE);
        },
        "stream-class-set-supports-discarded-packets:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_discarded_packets(
                selfComp.createTraceClass()->createStreamClass()->libObjPtr(), BT_TRUE, BT_FALSE);
        },
        "stream-class-set-supports-discarded-packets:supports-packets-for-discarded-packets-support",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_discarded_packets(
                createBasicStreamCls(selfComp)->libObjPtr(), BT_FALSE, BT_TRUE);
        },
        "stream-class-set-supports-discarded-packets:supports-discarded-packets-for-default-clock-snapshots",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = selfComp.createTraceClass()->createStreamClass();

            streamCls->supportsPackets(true, false, false);
            bt_stream_class_set_supports_discarded_packets(streamCls->libObjPtr(), BT_TRUE,
                                                           BT_TRUE);
        },
        "stream-class-set-supports-discarded-packets:has-default-clock-class-for-default-clock-snapshots",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_packets(createFrozenStreamCls(selfComp)->libObjPtr(),
                                                 BT_TRUE, BT_FALSE, BT_FALSE);
        },
        "stream-class-set-supports-packets:not-frozen:stream-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_packets(
                selfComp.createTraceClass()->createStreamClass()->libObjPtr(), BT_FALSE, BT_TRUE,
                BT_FALSE);
        },
        "stream-class-set-supports-packets:supports-packets-for-default-clock-snapshot", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_supports_packets(
                selfComp.createTraceClass()->createStreamClass()->libObjPtr(), BT_TRUE, BT_TRUE,
                BT_FALSE);
        },
        "stream-class-set-supports-packets:has-default-clock-class-for-default-clock-snapshot", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            streamCls->packetContextFieldClass(
                *streamCls->traceClass().createStructureFieldClass());
            bt_stream_class_set_supports_packets(streamCls->libObjPtr(), BT_FALSE, BT_FALSE,
                                                 BT_FALSE);
        },
        "stream-class-set-supports-packets:supports-packets-for-packet-context-field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            streamCls->supportsDiscardedPackets(true, false);
            bt_stream_class_set_supports_packets(streamCls->libObjPtr(), BT_FALSE, BT_FALSE,
                                                 BT_FALSE);
        },
        "stream-class-set-supports-packets:supports-packets-for-discarded-packets-support", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_user_attributes(createBasicStreamCls(selfComp)->libObjPtr(),
                                                nullptr);
        },
        "stream-class-set-user-attributes:not-null:user-attributes-value-object", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_user_attributes(createBasicStreamCls(selfComp)->libObjPtr(),
                                                bt2::BoolValue::create()->libObjPtr());
        },
        "stream-class-set-user-attributes:is-map-value:user-attributes", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_user_attributes(createFrozenStreamCls(selfComp)->libObjPtr(),
                                                bt2::MapValue::create()->libObjPtr());
        },
        "stream-class-set-user-attributes:not-frozen:stream-class", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_stream_class_get_namespace(createBasicStreamCls(selfComp)->libObjPtr());
        },
        CondTrigger::Type::Pre, "stream-class-get-namespace:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_stream_class_set_namespace(createBasicStreamCls(selfComp)->libObjPtr(), "ns");
        },
        CondTrigger::Type::Pre, "stream-class-set-namespace:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_stream_class_get_uid(createBasicStreamCls(selfComp)->libObjPtr());
        },
        CondTrigger::Type::Pre, "stream-class-get-uid:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_stream_class_set_uid(createBasicStreamCls(selfComp)->libObjPtr(), "uid");
        },
        CondTrigger::Type::Pre, "stream-class-set-uid:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            /* Stream class doesn't support packets by default */
            bt_stream_class_set_packet_context_field_class(
                traceCls->createStreamClass()->libObjPtr(),
                traceCls->createStructureFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre, "stream-class-set-packet-context-field-class:supports-packets", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_stream_class_set_packet_context_field_class(
                createBasicStreamCls(selfComp)->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre, "stream-class-set-packet-context-field-class:not-null:field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            /* Freeze the stream class by adding an event class */
            streamCls->createEventClass();
            bt_stream_class_set_packet_context_field_class(
                streamCls->libObjPtr(),
                streamCls->traceClass().createStructureFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "stream-class-set-packet-context-field-class:not-frozen:stream-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            bt_stream_class_set_packet_context_field_class(
                streamCls->libObjPtr(),
                streamCls->traceClass().createUnsignedIntegerFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "stream-class-set-packet-context-field-class:is-structure-field-class:field-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_stream_class_set_event_common_context_field_class(
                createBasicStreamCls(selfComp)->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "stream-class-set-event-common-context-field-class:not-null:field-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            /* Freeze the stream class */
            streamCls->createEventClass();
            bt_stream_class_set_event_common_context_field_class(
                streamCls->libObjPtr(),
                streamCls->traceClass().createStructureFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "stream-class-set-event-common-context-field-class:not-frozen:stream-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            bt_stream_class_set_event_common_context_field_class(
                streamCls->libObjPtr(),
                streamCls->traceClass().createUnsignedIntegerFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "stream-class-set-event-common-context-field-class:is-structure-field-class:field-class",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_namespace(createBasicStreamCls(selfComp)->libObjPtr(), nullptr);
        },
        "stream-class-set-namespace:not-null:namespace");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_namespace(createFrozenStreamCls(selfComp)->libObjPtr(), "ns");
        },
        "stream-class-set-namespace:not-frozen:stream-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_uid(createBasicStreamCls(selfComp)->libObjPtr(), nullptr);
        },
        "stream-class-set-uid:not-null:name");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_stream_class_set_uid(createFrozenStreamCls(selfComp)->libObjPtr(), "uid");
        },
        "stream-class-set-uid:not-frozen:stream-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->assignsAutomaticStreamClassId(true);

            withCurrentThreadError([&] {
                bt_stream_class_create(traceCls->libObjPtr());
            });
        },
        "stream-class-create:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->assignsAutomaticStreamClassId(false);

            withCurrentThreadError([&] {
                bt_stream_class_create_with_id(traceCls->libObjPtr(), 0);
            });
        },
        "stream-class-create-with-id:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->assignsAutomaticStreamClassId(false);
            bt_stream_class_create_with_id(traceCls->libObjPtr(), 42);
            bt_stream_class_create_with_id(traceCls->libObjPtr(), 42);
        },
        "stream-class-create-with-id:stream-class-id-is-unique", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            withCurrentThreadError([&] {
                bt_stream_class_set_name(streamCls->libObjPtr(), "name");
            });
        },
        "stream-class-set-name:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = selfComp.createTraceClass()->createStreamClass();
            const auto clockCls = selfComp.createClockClass();

            withCurrentThreadError([&] {
                bt_stream_class_set_default_clock_class(streamCls->libObjPtr(),
                                                        clockCls->libObjPtr());
            });
        },
        "stream-class-set-default-clock-class:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);
            const auto fc = streamCls->traceClass().createStructureFieldClass();

            withCurrentThreadError([&] {
                bt_stream_class_set_event_common_context_field_class(streamCls->libObjPtr(),
                                                                     fc->libObjPtr());
            });
        },
        "stream-class-set-event-common-context-field-class:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);
            const auto fc = streamCls->traceClass().createStructureFieldClass();

            withCurrentThreadError([&] {
                bt_stream_class_set_packet_context_field_class(streamCls->libObjPtr(),
                                                               fc->libObjPtr());
            });
        },
        "stream-class-set-packet-context-field-class:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            withCurrentThreadError([&] {
                bt_stream_class_set_namespace(streamCls->libObjPtr(), "ns");
            });
        },
        "stream-class-set-namespace:no-error");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = createBasicStreamCls(selfComp);

            withCurrentThreadError([&] {
                bt_stream_class_set_uid(streamCls->libObjPtr(), "uid");
            });
        },
        "stream-class-set-uid:no-error");
}
