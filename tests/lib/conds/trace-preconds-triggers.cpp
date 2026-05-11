/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/mip.hpp"
#include "cpp-common/bt2/trace-ir.hpp"
#include "cpp-common/bt2/value.hpp"
#include "cpp-common/bt2c/uuid.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

/*
 * Creates and returns a freshly instantiated trace from `selfComp`,
 * freezing it by instantiating a stream so that the
 * `BT_ASSERT_PRE_DEV_TRACE_HOT` check will trip on subsequent
 * mutating calls.
 */
bt2::Trace::Shared createFrozenTrace(const bt2::SelfComponent selfComp)
{
    const auto traceCls = selfComp.createTraceClass();
    const auto trace = traceCls->instantiate();

    /*
     * Instantiating a stream calls `bt_trace_add_stream()`, which
     * freezes the trace.
     */
    traceCls->createStreamClass()->instantiate(*trace);
    return trace;
}

void noopDestructionListener(const bt_trace *, void *) noexcept
{
}

} /* namespace */

/*
 * Adds trace API precondition failure triggers.
 */
void addTracePreCondsTriggers(CondTriggers& triggers)
{
    static const bt2c::Uuid validUuid {"9cc63e6b-8357-4935-a9b6-c4a7214fea12"};

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            withCurrentThreadError([&] {
                bt_trace_create(traceCls->libObjPtr());
            });
        },
        "trace-create:no-error", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_trace_set_namespace(nullptr, "ns");
        },
        "trace-set-namespace:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_set_name(nullptr, "name");
        },
        "trace-set-name:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_set_uuid(nullptr, validUuid.data());
        },
        "trace-set-uuid:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_set_uid(nullptr, "uid");
        },
        "trace-set-uid:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_set_environment_entry_string(nullptr, "name", "value");
        },
        "trace-set-environment-entry-string:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_set_environment_entry_integer(nullptr, "name", 42);
        },
        "trace-set-environment-entry-integer:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_listener_id id;

            bt_trace_add_destruction_listener(nullptr, noopDestructionListener, nullptr, &id);
        },
        "trace-add-destruction-listener:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_remove_destruction_listener(nullptr, 0);
        },
        "trace-remove-destruction-listener:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            const auto attrs = bt2::MapValue::create();

            bt_trace_set_user_attributes(nullptr, attrs->libObjPtr());
        },
        "trace-set-user-attributes:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_get_namespace(nullptr);
        },
        "trace-get-namespace:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_get_name(nullptr);
        },
        "trace-get-name:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_get_uuid(nullptr);
        },
        "trace-get-uuid:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_get_uid(nullptr);
        },
        "trace-get-uid:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_get_environment_entry_count(nullptr);
        },
        "trace-get-environment-entry-count:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            const char *name;
            const bt_value *value;

            bt_trace_borrow_environment_entry_by_index_const(nullptr, 0, &name, &value);
        },
        "trace-borrow-environment-entry-by-index-const:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_borrow_environment_entry_value_by_name_const(nullptr, "name");
        },
        "trace-borrow-environment-entry-value-by-name-const:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_get_stream_count(nullptr);
        },
        "trace-get-stream-count:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_borrow_stream_by_index(nullptr, 0);
        },
        "trace-borrow-stream-by-index:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_borrow_stream_by_id(nullptr, 0);
        },
        "trace-borrow-stream-by-id:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_borrow_class(nullptr);
        },
        "trace-borrow-class:not-null:trace");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_borrow_user_attributes_const(nullptr);
        },
        "trace-borrow-user-attributes-const:not-null:trace");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_name(selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr);
        },
        "trace-set-name:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_name(createFrozenTrace(selfComp)->libObjPtr(), "name");
        },
        "trace-set-name:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_uuid(selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr);
        },
        "trace-set-uuid:not-null:uuid", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_uuid(createFrozenTrace(selfComp)->libObjPtr(), validUuid.data());
        },
        "trace-set-uuid:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_namespace(selfComp.createTraceClass()->instantiate()->libObjPtr(),
                                   nullptr);
        },
        "trace-set-namespace:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_namespace(createFrozenTrace(selfComp)->libObjPtr(), "ns");
        },
        "trace-set-namespace:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_uid(createFrozenTrace(selfComp)->libObjPtr(), "uid");
        },
        "trace-set-uid:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_user_attributes(selfComp.createTraceClass()->instantiate()->libObjPtr(),
                                         nullptr);
        },
        "trace-set-user-attributes:not-null:user-attributes-value-object", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();

            bt_trace_set_user_attributes(trace->libObjPtr(),
                                         bt2::ArrayValue::create()->libObjPtr());
        },
        "trace-set-user-attributes:is-map-value:user-attributes", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = createFrozenTrace(selfComp);

            bt_trace_set_user_attributes(trace->libObjPtr(), bt2::MapValue::create()->libObjPtr());
        },
        "trace-set-user-attributes:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_environment_entry_string(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr, "value");
        },
        "trace-set-environment-entry-string:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_environment_entry_string(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), "name", nullptr);
        },
        "trace-set-environment-entry-string:not-null:value", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_set_environment_entry_integer(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr, 42);
        },
        "trace-set-environment-entry-integer:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const bt_value *value;

            bt_trace_borrow_environment_entry_by_index_const(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), 0, nullptr, &value);
        },
        "trace-borrow-environment-entry-by-index-const:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const char *name;

            bt_trace_borrow_environment_entry_by_index_const(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), 0, &name, nullptr);
        },
        "trace-borrow-environment-entry-by-index-const:not-null:value-object-output", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const char *name;
            const bt_value *value;

            bt_trace_borrow_environment_entry_by_index_const(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), 0, &name, &value);
        },
        "trace-borrow-environment-entry-by-index-const:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_borrow_environment_entry_value_by_name_const(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr);
        },
        "trace-borrow-environment-entry-value-by-name-const:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_borrow_stream_by_index(selfComp.createTraceClass()->instantiate()->libObjPtr(),
                                            0);
        },
        "trace-borrow-stream-by-index:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_listener_id id;

            bt_trace_add_destruction_listener(
                selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr, nullptr, &id);
        },
        "trace-add-destruction-listener:not-null:listener-function", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();
            bt_listener_id id;

            /*
             * Add a real listener so that the listener array exists,
             * then try to remove a different ID that we never added.
             */
            bt_trace_add_destruction_listener(trace->libObjPtr(), noopDestructionListener, nullptr,
                                              &id);
            bt_trace_remove_destruction_listener(trace->libObjPtr(), id + 1);
        },
        "trace-remove-destruction-listener:listener-id-exists", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_get_namespace(selfComp.createTraceClass()->instantiate()->libObjPtr());
        },
        CondTrigger::Type::Pre, "trace-get-namespace:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_set_namespace(selfComp.createTraceClass()->instantiate()->libObjPtr(), "ns");
        },
        CondTrigger::Type::Pre, "trace-set-namespace:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_get_uid(selfComp.createTraceClass()->instantiate()->libObjPtr());
        },
        CondTrigger::Type::Pre, "trace-get-uid:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_set_uid(selfComp.createTraceClass()->instantiate()->libObjPtr(), "uid");
        },
        CondTrigger::Type::Pre, "trace-set-uid:mip-version-is-valid", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_get_uuid(selfComp.createTraceClass()->instantiate()->libObjPtr());
        },
        CondTrigger::Type::Pre, "trace-get-uuid:mip-version-is-valid", 1));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_set_uuid(selfComp.createTraceClass()->instantiate()->libObjPtr(),
                              validUuid.data());
        },
        CondTrigger::Type::Pre, "trace-set-uuid:mip-version-is-valid", 1));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_trace_set_uid(selfComp.createTraceClass()->instantiate()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre, "trace-set-uid:not-null:uid", 1));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();

            withCurrentThreadError([&] {
                bt_trace_set_name(trace->libObjPtr(), "name");
            });
        },
        "trace-set-name:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();

            withCurrentThreadError([&] {
                bt_trace_set_namespace(trace->libObjPtr(), "ns");
            });
        },
        "trace-set-namespace:no-error", 1);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();

            withCurrentThreadError([&] {
                bt_trace_set_environment_entry_string(trace->libObjPtr(), "name", "value");
            });
        },
        "trace-set-environment-entry-string:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto trace = traceCls->instantiate();

            bt_trace_set_environment_entry_string(trace->libObjPtr(), "name", "value");
            traceCls->createStreamClass()->instantiate(*trace);
            bt_trace_set_environment_entry_string(trace->libObjPtr(), "name", "value");
        },
        "trace-set-environment-entry-string:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();

            withCurrentThreadError([&] {
                bt_trace_set_environment_entry_integer(trace->libObjPtr(), "name", 42);
            });
        },
        "trace-set-environment-entry-integer:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto trace = traceCls->instantiate();

            bt_trace_set_environment_entry_integer(trace->libObjPtr(), "name", 42);
            traceCls->createStreamClass()->instantiate(*trace);
            bt_trace_set_environment_entry_integer(trace->libObjPtr(), "name", 42);
        },
        "trace-set-environment-entry-integer:not-frozen:trace", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();

            withCurrentThreadError([&] {
                bt_listener_id id;

                bt_trace_add_destruction_listener(trace->libObjPtr(), noopDestructionListener,
                                                  nullptr, &id);
            });
        },
        "trace-add-destruction-listener:no-error", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto trace = selfComp.createTraceClass()->instantiate();
            bt_listener_id id;

            bt_trace_add_destruction_listener(trace->libObjPtr(), noopDestructionListener, nullptr,
                                              &id);

            withCurrentThreadError([&] {
                bt_trace_remove_destruction_listener(trace->libObjPtr(), id);
            });
        },
        "trace-remove-destruction-listener:no-error", 0);
}
