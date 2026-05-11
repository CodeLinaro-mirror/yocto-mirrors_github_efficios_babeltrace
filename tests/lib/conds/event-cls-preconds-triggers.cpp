/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <cstdint>
#include <functional>
#include <string_view>
#include <utility>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/mip.hpp"
#include "cpp-common/bt2/trace-ir.hpp"
#include "cpp-common/bt2/value.hpp"
#include "cpp-common/bt2/wrap.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

using FrozenEventClsTripFunc = std::function<void(bt_event_class *)>;

/*
 * Adds a "frozen event class" trigger built from `tripFunc`.
 *
 * The helper does the bare minimum graph plumbing needed to freeze a
 * freshly created event class (by emitting an event message for it),
 * then calls `tripFunc` on that frozen event class to exercise the
 * triggering API call.
 */
void addFrozenTrigger(CondTriggers& triggers, FrozenEventClsTripFunc tripFunc,
                      const std::string& condId, const std::uint64_t mipVersion,
                      const std::string_view nameSuffix = {})
{
    triggers.emplace_back(makeRunInMsgIterNextTrigger(
        [tripFunc = std::move(tripFunc)](const auto selfMsgIter, auto& msgs) {
            const auto traceCls = selfMsgIter.component().createTraceClass();
            const auto streamCls = traceCls->createStreamClass();
            const auto eventCls = streamCls->createEventClass();

            /*
             * Creating an event message freezes its event class. Append
             * the event message to keep its lifetime tied to the array,
             * so it stays alive while the triggering call runs.
             */
            msgs.append(selfMsgIter.createEventMessage(
                *eventCls, *streamCls->instantiate(*traceCls->instantiate())));

            /* Now exercise the triggering call on the frozen event class */
            tripFunc(eventCls->libObjPtr());
        },
        CondTrigger::Type::Pre, condId, mipVersion, nameSuffix));
}

} /* namespace */

/*
 * Adds event API precondition failure triggers.
 */
void addEventClsPreCondsTriggers(CondTriggers& triggers)
{
    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_create(nullptr);
        },
        "event-class-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_create_with_id(nullptr, 0);
        },
        "event-class-create-with-id:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_set_namespace(nullptr, nullptr);
        },
        "event-class-set-namespace:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_set_name(nullptr, nullptr);
        },
        "event-class-set-name:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_set_uid(nullptr, nullptr);
        },
        "event-class-set-uid:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_set_emf_uri(nullptr, nullptr);
        },
        "event-class-set-emf-uri:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_set_specific_context_field_class(nullptr, nullptr);
        },
        "event-class-set-specific-context-field-class:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_event_class_set_payload_field_class(nullptr, nullptr);
        },
        "event-class-set-payload-field-class:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_create(nullptr);
        },
        "event-class-create:not-null:stream-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_get_namespace(nullptr);
        },
        "event-class-get-namespace:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_namespace(nullptr, "ns");
        },
        "event-class-set-namespace:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_get_name(nullptr);
        },
        "event-class-get-name:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_name(nullptr, "name");
        },
        "event-class-set-name:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_get_uid(nullptr);
        },
        "event-class-get-uid:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_uid(nullptr, "uid");
        },
        "event-class-set-uid:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_get_id(nullptr);
        },
        "event-class-get-id:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_log_level dummy;

            bt_event_class_get_log_level(nullptr, &dummy);
        },
        "event-class-get-log-level:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_log_level(nullptr, BT_EVENT_CLASS_LOG_LEVEL_INFO);
        },
        "event-class-set-log-level:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_get_emf_uri(nullptr);
        },
        "event-class-get-emf-uri:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_emf_uri(nullptr, "uri");
        },
        "event-class-set-emf-uri:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_borrow_stream_class(nullptr);
        },
        "event-class-borrow-stream-class:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_borrow_specific_context_field_class(nullptr);
        },
        "event-class-borrow-specific-context-field-class:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_specific_context_field_class(nullptr, nullptr);
        },
        "event-class-set-specific-context-field-class:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_borrow_payload_field_class(nullptr);
        },
        "event-class-borrow-payload-field-class:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_payload_field_class(nullptr, nullptr);
        },
        "event-class-set-payload-field-class:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_borrow_user_attributes_const(nullptr);
        },
        "event-class-borrow-user-attributes-const:not-null:event-class");

    addPreTrigger(
        triggers,
        [] {
            bt_event_class_set_user_attributes(nullptr, nullptr);
        },
        "event-class-set-user-attributes:not-null:event-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = selfComp.createTraceClass()->createStreamClass();

            streamCls->assignsAutomaticEventClassId(false);
            bt_event_class_create(streamCls->libObjPtr());
        },
        "event-class-create:stream-class-automatically-assigns-event-class-ids", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            /* Default is automatic */
            bt_event_class_create_with_id(
                selfComp.createTraceClass()->createStreamClass()->libObjPtr(), 0);
        },
        "event-class-create-with-id:stream-class-does-not-automatically-assigns-event-class-ids",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto streamCls = selfComp.createTraceClass()->createStreamClass();

            streamCls->assignsAutomaticEventClassId(false);
            bt_event_class_create_with_id(streamCls->libObjPtr(), 42);
            bt_event_class_create_with_id(streamCls->libObjPtr(), 42);
        },
        "event-class-create-with-id:event-class-id-is-unique", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_name(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-name:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_get_log_level(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-get-log-level:not-null:log-level-output", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_emf_uri(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-emf-uri:not-null:emf-uri", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_specific_context_field_class(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-specific-context-field-class:not-null:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_event_class_set_specific_context_field_class(
                traceCls->createStreamClass()->createEventClass()->libObjPtr(),
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr());
        },
        "event-class-set-specific-context-field-class:is-structure-field-class:specific-context",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto innerStructFc = traceCls->createStructureFieldClass();
            const auto outerStructFc = traceCls->createStructureFieldClass();

            outerStructFc->appendMember("inner", *innerStructFc);
            bt_event_class_set_specific_context_field_class(
                traceCls->createStreamClass()->createEventClass()->libObjPtr(),
                innerStructFc->libObjPtr());
        },
        "event-class-set-specific-context-field-class:field-class-is-not-part-of-something", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto lenFc = traceCls->createUnsignedIntegerFieldClass();
            const auto contextFc = traceCls->createStructureFieldClass();

            contextFc->appendMember("array",
                                    *traceCls->createDynamicArrayFieldClass(
                                        *traceCls->createUnsignedIntegerFieldClass(), *lenFc));
            contextFc->appendMember("length", *lenFc);
            bt_event_class_set_specific_context_field_class(
                traceCls->createStreamClass()->createEventClass()->libObjPtr(),
                contextFc->libObjPtr());
        },
        CondTrigger::Type::Pre, "event-class-set-specific-context-field-class:valid-field-class",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_payload_field_class(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-payload-field-class:not-null:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_event_class_set_payload_field_class(
                traceCls->createStreamClass()->createEventClass()->libObjPtr(),
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr());
        },
        "event-class-set-payload-field-class:is-structure-field-class:payload", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto innerStructFc = traceCls->createStructureFieldClass();
            const auto outerStructFc = traceCls->createStructureFieldClass();

            outerStructFc->appendMember("inner", *innerStructFc);
            bt_event_class_set_payload_field_class(
                traceCls->createStreamClass()->createEventClass()->libObjPtr(),
                innerStructFc->libObjPtr());
        },
        "event-class-set-payload-field-class:field-class-is-not-part-of-something", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto lenFc = traceCls->createUnsignedIntegerFieldClass();
            const auto payloadFc = traceCls->createStructureFieldClass();

            payloadFc->appendMember("array",
                                    *traceCls->createDynamicArrayFieldClass(
                                        *traceCls->createUnsignedIntegerFieldClass(), *lenFc));
            payloadFc->appendMember("length", *lenFc);
            bt_event_class_set_payload_field_class(
                traceCls->createStreamClass()->createEventClass()->libObjPtr(),
                payloadFc->libObjPtr());
        },
        CondTrigger::Type::Pre, "event-class-set-payload-field-class:valid-field-class", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_user_attributes(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-user-attributes:not-null:user-attributes-value-object", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_user_attributes(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                bt2::BoolValue::create()->libObjPtr());
        },
        "event-class-set-user-attributes:is-map-value:user-attributes", 0);

    for (std::uint64_t mipVersion = 0; mipVersion <= bt2::getMaximalMipVersion(); ++mipVersion) {
        const auto suffix = fmt::format("mip{}", mipVersion);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                bt_event_class_set_name(libEventClsPtr, "x");
            },
            "event-class-set-name:not-frozen:event-class", mipVersion, suffix);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                bt_event_class_set_log_level(libEventClsPtr, BT_EVENT_CLASS_LOG_LEVEL_INFO);
            },
            "event-class-set-log-level:not-frozen:event-class", mipVersion, suffix);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                bt_event_class_set_emf_uri(libEventClsPtr, "x");
            },
            "event-class-set-emf-uri:not-frozen:event-class", mipVersion, suffix);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                /*
                 * Field class is irrelevant: the `not-frozen` check is
                 * performed before the field class is inspected (after
                 * the `non-null` check, which a structure field class
                 * created here would pass).
                 */
                const auto eventCls = bt2::wrap(libEventClsPtr);

                eventCls.specificContextFieldClass(
                    *eventCls.streamClass().traceClass().createStructureFieldClass());
            },
            "event-class-set-specific-context-field-class:not-frozen:event-class", mipVersion,
            suffix);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                const auto eventCls = bt2::wrap(libEventClsPtr);

                eventCls.payloadFieldClass(
                    *eventCls.streamClass().traceClass().createStructureFieldClass());
            },
            "event-class-set-payload-field-class:not-frozen:event-class", mipVersion, suffix);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                bt_event_class_set_user_attributes(libEventClsPtr,
                                                   bt2::MapValue::create()->libObjPtr());
            },
            "event-class-set-user-attributes:not-frozen:event-class", mipVersion, suffix);
    }

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_event_class_get_namespace(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr());
        },
        CondTrigger::Type::Pre, "event-class-get-namespace:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_event_class_set_namespace(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                "ns");
        },
        CondTrigger::Type::Pre, "event-class-set-namespace:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_event_class_get_uid(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr());
        },
        CondTrigger::Type::Pre, "event-class-get-uid:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_event_class_set_uid(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                "uid");
        },
        CondTrigger::Type::Pre, "event-class-set-uid:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_namespace(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-namespace:not-null:namespace");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_event_class_set_uid(
                selfComp.createTraceClass()->createStreamClass()->createEventClass()->libObjPtr(),
                nullptr);
        },
        "event-class-set-uid:not-null:name");

    for (std::uint64_t mipVersion = 1; mipVersion <= bt2::getMaximalMipVersion(); ++mipVersion) {
        const auto suffix = fmt::format("mip{}", mipVersion);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                bt_event_class_set_namespace(libEventClsPtr, "x");
            },
            "event-class-set-namespace:not-frozen:event-class", mipVersion, suffix);

        addFrozenTrigger(
            triggers,
            [](const auto libEventClsPtr) {
                bt_event_class_set_uid(libEventClsPtr, "x");
            },
            "event-class-set-uid:not-frozen:event-class", mipVersion, suffix);
    }
}
