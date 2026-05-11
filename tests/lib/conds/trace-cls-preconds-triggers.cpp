/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/value.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Adds trace class API precondition failure triggers.
 */
void addTraceClassPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_create(nullptr);
        },
        "trace-class-create:not-null:component");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_trace_class_create(nullptr);
        },
        "trace-class-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_add_destruction_listener(
                nullptr,
                [](const auto, auto) {
                },
                nullptr, nullptr);
        },
        "trace-class-add-destruction-listener:not-null:trace-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_trace_class_add_destruction_listener(
                nullptr,
                [](const auto, auto) {
                },
                nullptr, nullptr);
        },
        "trace-class-add-destruction-listener:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_remove_destruction_listener(nullptr, 0);
        },
        "trace-class-remove-destruction-listener:not-null:trace-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_trace_class_remove_destruction_listener(nullptr, 0);
        },
        "trace-class-remove-destruction-listener:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_get_stream_class_count(nullptr);
        },
        "trace-class-get-stream-class-count:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_borrow_stream_class_by_index(nullptr, 0);
        },
        "trace-class-borrow-stream-class-by-index:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_borrow_stream_class_by_id(nullptr, 0);
        },
        "trace-class-borrow-stream-class-by-id:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_assigns_automatic_stream_class_id(nullptr);
        },
        "trace-class-assigns-automatic-stream-class-id:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_set_assigns_automatic_stream_class_id(nullptr, BT_TRUE);
        },
        "trace-class-set-assigns-automatic-stream-class-id:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_borrow_user_attributes_const(nullptr);
        },
        "trace-class-borrow-user-attributes-const:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_get_graph_mip_version(nullptr);
        },
        "trace-class-get-graph-mip-version:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_trace_class_set_user_attributes(nullptr, nullptr);
        },
        "trace-class-set-user-attributes:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_class_add_destruction_listener(selfComp.createTraceClass()->libObjPtr(),
                                                    nullptr, nullptr, nullptr);
        },
        "trace-class-add-destruction-listener:not-null:listener-function", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_class_remove_destruction_listener(selfComp.createTraceClass()->libObjPtr(), 0);
        },
        "trace-class-remove-destruction-listener:listener-id-exists", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_class_borrow_stream_class_by_index(selfComp.createTraceClass()->libObjPtr(),
                                                        0);
        },
        "trace-class-borrow-stream-class-by-index:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->createStreamClass();
            bt_trace_class_set_assigns_automatic_stream_class_id(traceCls->libObjPtr(), BT_TRUE);
        },
        "trace-class-set-assigns-automatic-stream-class-id:not-frozen:trace-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_class_set_user_attributes(selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        "trace-class-set-user-attributes:not-null:user-attributes-value-object", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_trace_class_set_user_attributes(selfComp.createTraceClass()->libObjPtr(),
                                               bt2::BoolValue::create()->libObjPtr());
        },
        "trace-class-set-user-attributes:is-map-value:user-attributes", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            traceCls->createStreamClass();
            bt_trace_class_set_user_attributes(traceCls->libObjPtr(),
                                               bt2::MapValue::create()->libObjPtr());
        },
        "trace-class-set-user-attributes:not-frozen:trace-class", 0);
}
