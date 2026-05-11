/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/field-class.hpp"
#include "cpp-common/bt2/self-component-port.hpp"
#include "cpp-common/bt2/trace-ir.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Adds field path API precondition failure triggers.
 */
void addFieldPathPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_field_path_get_root_scope(nullptr);
        },
        "field-path-get-root-scope:not-null:field-path");

    addPreTrigger(
        triggers,
        [] {
            bt_field_path_get_item_count(nullptr);
        },
        "field-path-get-item-count:not-null:field-path");

    addPreTrigger(
        triggers,
        [] {
            bt_field_path_borrow_item_by_index_const(nullptr, 0);
        },
        "field-path-borrow-item-by-index-const:not-null:field-path");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto streamCls = traceCls->createStreamClass();
            const auto payloadFc = traceCls->createStructureFieldClass();
            const auto lenFc = traceCls->createUnsignedIntegerFieldClass();
            const auto dynArrayFc = traceCls->createDynamicArrayFieldClass(
                *traceCls->createUnsignedIntegerFieldClass(), *lenFc);

            payloadFc->appendMember("len", *lenFc);
            payloadFc->appendMember("arr", *dynArrayFc);
            streamCls->createEventClass()->payloadFieldClass(*payloadFc);

            const auto fieldPath =
                bt_field_class_array_dynamic_with_length_field_borrow_length_field_path_const(
                    dynArrayFc->libObjPtr());

            bt_field_path_borrow_item_by_index_const(fieldPath, 1);
        },
        CondTrigger::Type::Pre, "field-path-borrow-item-by-index-const:valid-index", 0));

    addPreTrigger(
        triggers,
        [] {
            bt_field_path_item_get_type(nullptr);
        },
        "field-path-item-get-type:not-null:field-path-item");

    addPreTrigger(
        triggers,
        [] {
            bt_field_path_item_index_get_index(nullptr);
        },
        "field-path-item-index-get-index:not-null:field-path-item");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto payloadFc = traceCls->createStructureFieldClass();
            const auto outerLenFc = traceCls->createUnsignedIntegerFieldClass();
            const auto elemFc = traceCls->createStructureFieldClass();
            const auto innerLenFc = traceCls->createUnsignedIntegerFieldClass();
            const auto innerDynArrayFc = traceCls->createDynamicArrayFieldClass(
                *traceCls->createUnsignedIntegerFieldClass(), *innerLenFc);

            elemFc->appendMember("len", *innerLenFc);
            elemFc->appendMember("arr", *innerDynArrayFc);
            payloadFc->appendMember("len", *outerLenFc);
            payloadFc->appendMember("arr",
                                    *traceCls->createDynamicArrayFieldClass(*elemFc, *outerLenFc));
            traceCls->createStreamClass()->createEventClass()->payloadFieldClass(*payloadFc);

            const auto fieldPath =
                bt_field_class_array_dynamic_with_length_field_borrow_length_field_path_const(
                    innerDynArrayFc->libObjPtr());

            bt_field_path_item_index_get_index(
                bt_field_path_borrow_item_by_index_const(fieldPath, 1));
        },
        CondTrigger::Type::Pre, "field-path-item-index-get-index:is-index-field-path-item", 0));
}
