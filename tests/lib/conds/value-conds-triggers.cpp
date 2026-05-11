/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/clock-class.hpp"
#include "cpp-common/bt2/value.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

/*
 * "For each" user function which the postcondition triggers use:
 * appends a cause to the error of the current thread and returns the
 * success status to trip the `no-error-if-no-error-status`
 * postcondition.
 */
bt_value_map_foreach_entry_func_status appendErrorCauseAndReturnOk(const char *, bt_value * const,
                                                                   void * const) noexcept
{
    BT_CURRENT_THREAD_ERROR_APPEND_CAUSE_FROM_UNKNOWN("cond-trigger", "Trigger error.");
    return BT_VALUE_MAP_FOREACH_ENTRY_FUNC_STATUS_OK;
}

/*
 * `const` version of appendErrorCauseAndReturnOk().
 */
bt_value_map_foreach_entry_const_func_status
appendErrorCauseAndReturnOkConst(const char *, const bt_value * const, void * const) noexcept
{
    BT_CURRENT_THREAD_ERROR_APPEND_CAUSE_FROM_UNKNOWN("cond-trigger", "Trigger error.");
    return BT_VALUE_MAP_FOREACH_ENTRY_CONST_FUNC_STATUS_OK;
}

/*
 * Appends a `not-frozen:value-object` precondition trigger.
 *
 * `createValFunc` creates the value to freeze (and may populate it with
 * elements first).
 *
 * The trigger wraps the value in a fresh map value, sets that map as
 * the user attributes of a clock class, and attaches that clock class
 * as the default clock class of a stream class: attaching the clock
 * class freezes it, which freezes its user attributes recursively,
 * which in turn freezes the value.
 *
 * `tripFunc` receives a `bt_value *` object which is the now-frozen
 * value and calls the target API function to trip its
 * `not-frozen:value-object` precondition.
 */
template <typename ValFactoryFunc, typename TripFunc>
void addFrozenValTrigger(CondTriggers& triggers, ValFactoryFunc createValFunc, TripFunc tripFunc,
                         const std::string& condId)
{
    triggers.emplace_back(makeRunInCompInitTrigger(
        [createValFunc = std::move(createValFunc),
         tripFunc = std::move(tripFunc)](const auto selfComp) {
            const auto val = createValFunc();
            const auto wrapperVal = bt2::MapValue::create();

            wrapperVal->insert("v", *val);

            const auto clkCls = selfComp.createClockClass();

            clkCls->userAttributes(*wrapperVal);
            selfComp.createTraceClass()->createStreamClass()->defaultClockClass(*clkCls);
            tripFunc(val->libObjPtr());
        },
        CondTrigger::Type::Pre, condId, 0));
}

} /* namespace */

/*
 * Adds value API precondition and postcondition failure triggers.
 */
void addValueCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_value_get_type(nullptr);
        },
        "value-get-type:not-null:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_bool_create();
        },
        "value-bool-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_bool_create_init(BT_TRUE);
        },
        "value-bool-create-init:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_bool_set(nullptr, BT_TRUE);
        },
        "value-bool-set:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_bool_set(bt2::SignedIntegerValue::create()->libObjPtr(), BT_TRUE);
        },
        "value-bool-set:is-boolean-value:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_bool_get(nullptr);
        },
        "value-bool-get:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_bool_get(bt2::SignedIntegerValue::create()->libObjPtr());
        },
        "value-bool-get:is-boolean-value:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_integer_unsigned_create();
        },
        "value-integer-unsigned-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_integer_unsigned_create_init(0);
        },
        "value-integer-unsigned-create-init:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_unsigned_set(nullptr, 0);
        },
        "value-integer-unsigned-set:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_unsigned_set(bt2::BoolValue::create()->libObjPtr(), 0);
        },
        "value-integer-unsigned-set:is-unsigned-int-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::UnsignedIntegerValue::create();
        },
        [](const auto libValPtr) {
            bt_value_integer_unsigned_set(libValPtr, 0);
        },
        "value-integer-unsigned-set:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_unsigned_get(nullptr);
        },
        "value-integer-unsigned-get:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_unsigned_get(bt2::BoolValue::create()->libObjPtr());
        },
        "value-integer-unsigned-get:is-unsigned-int-value:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_integer_signed_create();
        },
        "value-integer-signed-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_integer_signed_create_init(0);
        },
        "value-integer-signed-create-init:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_signed_set(nullptr, 0);
        },
        "value-integer-signed-set:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_signed_set(bt2::BoolValue::create()->libObjPtr(), 0);
        },
        "value-integer-signed-set:is-signed-int-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::SignedIntegerValue::create();
        },
        [](const auto libValPtr) {
            bt_value_integer_signed_set(libValPtr, 0);
        },
        "value-integer-signed-set:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_signed_get(nullptr);
        },
        "value-integer-signed-get:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_integer_signed_get(bt2::BoolValue::create()->libObjPtr());
        },
        "value-integer-signed-get:is-signed-int-value:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_real_create();
        },
        "value-real-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_real_create_init(0.);
        },
        "value-real-create-init:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_real_set(nullptr, 0.);
        },
        "value-real-set:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_real_set(bt2::BoolValue::create()->libObjPtr(), 0.);
        },
        "value-real-set:is-real-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::RealValue::create();
        },
        [](const auto libValPtr) {
            bt_value_real_set(libValPtr, 0.);
        },
        "value-real-set:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_real_get(nullptr);
        },
        "value-real-get:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_real_get(bt2::BoolValue::create()->libObjPtr());
        },
        "value-real-get:is-real-value:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_string_create();
        },
        "value-string-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_string_create_init("");
        },
        "value-string-create-init:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_string_create_init(nullptr);
        },
        "value-string-create-init:not-null:raw-value");

    addPreTrigger(
        triggers,
        [] {
            const auto val = bt2::StringValue::create();

            withCurrentThreadError([&] {
                bt_value_string_set(val->libObjPtr(), "");
            });
        },
        "value-string-set:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_string_set(nullptr, "");
        },
        "value-string-set:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_string_set(bt2::BoolValue::create()->libObjPtr(), "");
        },
        "value-string-set:is-string-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::StringValue::create();
        },
        [](const auto libValPtr) {
            bt_value_string_set(libValPtr, "");
        },
        "value-string-set:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_string_get(nullptr);
        },
        "value-string-get:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_string_get(bt2::BoolValue::create()->libObjPtr());
        },
        "value-string-get:is-string-value:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_array_create();
        },
        "value-array-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();
            const auto elemVal = bt2::BoolValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_element(arrayVal->libObjPtr(), elemVal->libObjPtr());
            });
        },
        "value-array-append-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_element(nullptr, bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-append-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_element(bt2::ArrayValue::create()->libObjPtr(), nullptr);
        },
        "value-array-append-element:not-null:element-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_element(bt2::BoolValue::create()->libObjPtr(),
                                          bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-append-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_element(libArrayValPtr, bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-append-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_bool_element(arrayVal->libObjPtr(), BT_TRUE);
            });
        },
        "value-array-append-bool-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_bool_element(nullptr, BT_TRUE);
        },
        "value-array-append-bool-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_bool_element(bt2::BoolValue::create()->libObjPtr(), BT_TRUE);
        },
        "value-array-append-bool-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_bool_element(libArrayValPtr, BT_TRUE);
        },
        "value-array-append-bool-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_unsigned_integer_element(arrayVal->libObjPtr(), 0);
            });
        },
        "value-array-append-unsigned-integer-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_unsigned_integer_element(nullptr, 0);
        },
        "value-array-append-unsigned-integer-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_unsigned_integer_element(bt2::BoolValue::create()->libObjPtr(),
                                                           0);
        },
        "value-array-append-unsigned-integer-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_unsigned_integer_element(libArrayValPtr, 0);
        },
        "value-array-append-unsigned-integer-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_signed_integer_element(arrayVal->libObjPtr(), 0);
            });
        },
        "value-array-append-signed-integer-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_signed_integer_element(nullptr, 0);
        },
        "value-array-append-signed-integer-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_signed_integer_element(bt2::BoolValue::create()->libObjPtr(), 0);
        },
        "value-array-append-signed-integer-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_signed_integer_element(libArrayValPtr, 0);
        },
        "value-array-append-signed-integer-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_real_element(arrayVal->libObjPtr(), 0.);
            });
        },
        "value-array-append-real-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_real_element(nullptr, 0.);
        },
        "value-array-append-real-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_real_element(bt2::BoolValue::create()->libObjPtr(), 0.);
        },
        "value-array-append-real-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_real_element(libArrayValPtr, 0.);
        },
        "value-array-append-real-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_string_element(arrayVal->libObjPtr(), "");
            });
        },
        "value-array-append-string-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_string_element(nullptr, "");
        },
        "value-array-append-string-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_string_element(bt2::BoolValue::create()->libObjPtr(), "");
        },
        "value-array-append-string-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_string_element(libArrayValPtr, "");
        },
        "value-array-append-string-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_empty_array_element(arrayVal->libObjPtr(), nullptr);
            });
        },
        "value-array-append-empty-array-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_empty_array_element(nullptr, nullptr);
        },
        "value-array-append-empty-array-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_empty_array_element(bt2::BoolValue::create()->libObjPtr(),
                                                      nullptr);
        },
        "value-array-append-empty-array-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_empty_array_element(libArrayValPtr, nullptr);
        },
        "value-array-append-empty-array-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            withCurrentThreadError([&] {
                bt_value_array_append_empty_map_element(arrayVal->libObjPtr(), nullptr);
            });
        },
        "value-array-append-empty-map-element:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_empty_map_element(nullptr, nullptr);
        },
        "value-array-append-empty-map-element:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_append_empty_map_element(bt2::BoolValue::create()->libObjPtr(), nullptr);
        },
        "value-array-append-empty-map-element:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::ArrayValue::create();
        },
        [](const auto libArrayValPtr) {
            bt_value_array_append_empty_map_element(libArrayValPtr, nullptr);
        },
        "value-array-append-empty-map-element:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            arrayVal->append(*bt2::BoolValue::create());

            const auto elemVal = bt2::BoolValue::create();

            withCurrentThreadError([&] {
                bt_value_array_set_element_by_index(arrayVal->libObjPtr(), 0, elemVal->libObjPtr());
            });
        },
        "value-array-set-element-by-index:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_set_element_by_index(nullptr, 0, bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-set-element-by-index:not-null:array-value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            arrayVal->append(*bt2::BoolValue::create());
            bt_value_array_set_element_by_index(arrayVal->libObjPtr(), 0, nullptr);
        },
        "value-array-set-element-by-index:not-null:element-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_set_element_by_index(bt2::BoolValue::create()->libObjPtr(), 0,
                                                bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-set-element-by-index:is-array-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            const auto arrayVal = bt2::ArrayValue::create();

            arrayVal->append(*bt2::BoolValue::create());
            return arrayVal;
        },
        [](const auto libArrayValPtr) {
            bt_value_array_set_element_by_index(libArrayValPtr, 0,
                                                bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-set-element-by-index:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_set_element_by_index(bt2::ArrayValue::create()->libObjPtr(), 0,
                                                bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-set-element-by-index:valid-index");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_borrow_element_by_index(nullptr, 0);
        },
        "value-array-borrow-element-by-index:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_borrow_element_by_index(bt2::BoolValue::create()->libObjPtr(), 0);
        },
        "value-array-borrow-element-by-index:is-array-value:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_borrow_element_by_index(bt2::ArrayValue::create()->libObjPtr(), 0);
        },
        "value-array-borrow-element-by-index:valid-index");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_get_length(nullptr);
        },
        "value-array-get-length:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_array_get_length(bt2::BoolValue::create()->libObjPtr());
        },
        "value-array-get-length:is-array-value:value-object");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_value_map_create();
        },
        "value-map-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();
            const auto elemVal = bt2::BoolValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_entry(mapVal->libObjPtr(), "k", elemVal->libObjPtr());
            });
        },
        "value-map-insert-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_entry(nullptr, "k", bt2::BoolValue::create()->libObjPtr());
        },
        "value-map-insert-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_entry(bt2::MapValue::create()->libObjPtr(), nullptr,
                                      bt2::BoolValue::create()->libObjPtr());
        },
        "value-map-insert-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_entry(bt2::MapValue::create()->libObjPtr(), "k", nullptr);
        },
        "value-map-insert-entry:not-null:element-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_entry(bt2::BoolValue::create()->libObjPtr(), "k",
                                      bt2::BoolValue::create()->libObjPtr());
        },
        "value-map-insert-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_entry(libMapValPtr, "k", bt2::BoolValue::create()->libObjPtr());
        },
        "value-map-insert-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_bool_entry(mapVal->libObjPtr(), "k", BT_TRUE);
            });
        },
        "value-map-insert-bool-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_bool_entry(nullptr, "k", BT_TRUE);
        },
        "value-map-insert-bool-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_bool_entry(bt2::MapValue::create()->libObjPtr(), nullptr, BT_TRUE);
        },
        "value-map-insert-bool-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_bool_entry(bt2::BoolValue::create()->libObjPtr(), "k", BT_TRUE);
        },
        "value-map-insert-bool-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_bool_entry(libMapValPtr, "k", BT_TRUE);
        },
        "value-map-insert-bool-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_unsigned_integer_entry(mapVal->libObjPtr(), "k", 0);
            });
        },
        "value-map-insert-unsigned-integer-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_unsigned_integer_entry(nullptr, "k", 0);
        },
        "value-map-insert-unsigned-integer-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_unsigned_integer_entry(bt2::MapValue::create()->libObjPtr(),
                                                       nullptr, 0);
        },
        "value-map-insert-unsigned-integer-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_unsigned_integer_entry(bt2::BoolValue::create()->libObjPtr(), "k",
                                                       0);
        },
        "value-map-insert-unsigned-integer-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_unsigned_integer_entry(libMapValPtr, "k", 0);
        },
        "value-map-insert-unsigned-integer-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_signed_integer_entry(mapVal->libObjPtr(), "k", 0);
            });
        },
        "value-map-insert-signed-integer-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_signed_integer_entry(nullptr, "k", 0);
        },
        "value-map-insert-signed-integer-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_signed_integer_entry(bt2::MapValue::create()->libObjPtr(), nullptr,
                                                     0);
        },
        "value-map-insert-signed-integer-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_signed_integer_entry(bt2::BoolValue::create()->libObjPtr(), "k", 0);
        },
        "value-map-insert-signed-integer-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_signed_integer_entry(libMapValPtr, "k", 0);
        },
        "value-map-insert-signed-integer-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_real_entry(mapVal->libObjPtr(), "k", 0.);
            });
        },
        "value-map-insert-real-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_real_entry(nullptr, "k", 0.);
        },
        "value-map-insert-real-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_real_entry(bt2::MapValue::create()->libObjPtr(), nullptr, 0.);
        },
        "value-map-insert-real-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_real_entry(bt2::BoolValue::create()->libObjPtr(), "k", 0.);
        },
        "value-map-insert-real-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_real_entry(libMapValPtr, "k", 0.);
        },
        "value-map-insert-real-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_string_entry(mapVal->libObjPtr(), "k", "");
            });
        },
        "value-map-insert-string-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_string_entry(nullptr, "k", "");
        },
        "value-map-insert-string-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_string_entry(bt2::MapValue::create()->libObjPtr(), nullptr, "");
        },
        "value-map-insert-string-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_string_entry(bt2::BoolValue::create()->libObjPtr(), "k", "");
        },
        "value-map-insert-string-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_string_entry(libMapValPtr, "k", "");
        },
        "value-map-insert-string-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_empty_array_entry(mapVal->libObjPtr(), "k", nullptr);
            });
        },
        "value-map-insert-empty-array-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_empty_array_entry(nullptr, "k", nullptr);
        },
        "value-map-insert-empty-array-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_empty_array_entry(bt2::MapValue::create()->libObjPtr(), nullptr,
                                                  nullptr);
        },
        "value-map-insert-empty-array-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_empty_array_entry(bt2::BoolValue::create()->libObjPtr(), "k",
                                                  nullptr);
        },
        "value-map-insert-empty-array-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_empty_array_entry(libMapValPtr, "k", nullptr);
        },
        "value-map-insert-empty-array-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_insert_empty_map_entry(mapVal->libObjPtr(), "k", nullptr);
            });
        },
        "value-map-insert-empty-map-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_empty_map_entry(nullptr, "k", nullptr);
        },
        "value-map-insert-empty-map-entry:not-null:map-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_empty_map_entry(bt2::MapValue::create()->libObjPtr(), nullptr,
                                                nullptr);
        },
        "value-map-insert-empty-map-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_insert_empty_map_entry(bt2::BoolValue::create()->libObjPtr(), "k",
                                                nullptr);
        },
        "value-map-insert-empty-map-entry:is-map-value:value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libMapValPtr) {
            bt_value_map_insert_empty_map_entry(libMapValPtr, "k", nullptr);
        },
        "value-map-insert-empty-map-entry:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_borrow_entry_value(nullptr, "k");
        },
        "value-map-borrow-entry-value:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_borrow_entry_value(bt2::MapValue::create()->libObjPtr(), nullptr);
        },
        "value-map-borrow-entry-value:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_borrow_entry_value(bt2::BoolValue::create()->libObjPtr(), "k");
        },
        "value-map-borrow-entry-value:is-map-value:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_get_size(nullptr);
        },
        "value-map-get-size:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_get_size(bt2::BoolValue::create()->libObjPtr());
        },
        "value-map-get-size:is-map-value:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_has_entry(nullptr, "k");
        },
        "value-map-has-entry:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_has_entry(bt2::MapValue::create()->libObjPtr(), nullptr);
        },
        "value-map-has-entry:not-null:key");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_has_entry(bt2::BoolValue::create()->libObjPtr(), "k");
        },
        "value-map-has-entry:is-map-value:value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_foreach_entry(mapVal->libObjPtr(), appendErrorCauseAndReturnOk,
                                           nullptr);
            });
        },
        "value-map-foreach-entry:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_foreach_entry(nullptr, appendErrorCauseAndReturnOk, nullptr);
        },
        "value-map-foreach-entry:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_foreach_entry(bt2::MapValue::create()->libObjPtr(), nullptr, nullptr);
        },
        "value-map-foreach-entry:not-null:user-function");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_foreach_entry(bt2::BoolValue::create()->libObjPtr(),
                                       appendErrorCauseAndReturnOk, nullptr);
        },
        "value-map-foreach-entry:is-map-value:value-object");

    triggers.emplace_back(makeSimpleTrigger(
        [] {
            const auto mapVal = bt2::MapValue::create();

            mapVal->insert("k", false);
            bt_value_map_foreach_entry(mapVal->libObjPtr(), appendErrorCauseAndReturnOk, nullptr);
        },
        CondTrigger::Type::Post, "value-map-foreach-entry-func:no-error-if-no-error-status"));

    addPreTrigger(
        triggers,
        [] {
            const auto mapVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_foreach_entry_const(mapVal->libObjPtr(),
                                                 appendErrorCauseAndReturnOkConst, nullptr);
            });
        },
        "value-map-foreach-entry-const:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_foreach_entry_const(nullptr, appendErrorCauseAndReturnOkConst, nullptr);
        },
        "value-map-foreach-entry-const:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_foreach_entry_const(bt2::MapValue::create()->libObjPtr(), nullptr,
                                             nullptr);
        },
        "value-map-foreach-entry-const:not-null:user-function");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_foreach_entry_const(bt2::BoolValue::create()->libObjPtr(),
                                             appendErrorCauseAndReturnOkConst, nullptr);
        },
        "value-map-foreach-entry-const:is-map-value:value-object");

    triggers.emplace_back(makeSimpleTrigger(
        [] {
            const auto mapVal = bt2::MapValue::create();

            mapVal->insert("k", false);
            bt_value_map_foreach_entry_const(mapVal->libObjPtr(), appendErrorCauseAndReturnOkConst,
                                             nullptr);
        },
        CondTrigger::Type::Post, "value-map-foreach-entry-const-func:no-error-if-no-error-status"));

    addPreTrigger(
        triggers,
        [] {
            const auto baseVal = bt2::MapValue::create();
            const auto extVal = bt2::MapValue::create();

            withCurrentThreadError([&] {
                bt_value_map_extend(baseVal->libObjPtr(), extVal->libObjPtr());
            });
        },
        "value-map-extend:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_extend(nullptr, bt2::MapValue::create()->libObjPtr());
        },
        "value-map-extend:not-null:base-value-object");

    addFrozenValTrigger(
        triggers,
        [] {
            return bt2::MapValue::create();
        },
        [](const auto libBaseValPtr) {
            bt_value_map_extend(libBaseValPtr, bt2::MapValue::create()->libObjPtr());
        },
        "value-map-extend:not-frozen:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_extend(bt2::MapValue::create()->libObjPtr(), nullptr);
        },
        "value-map-extend:not-null:extension-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_extend(bt2::BoolValue::create()->libObjPtr(),
                                bt2::MapValue::create()->libObjPtr());
        },
        "value-map-extend:is-map-value:base-value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_map_extend(bt2::MapValue::create()->libObjPtr(),
                                bt2::BoolValue::create()->libObjPtr());
        },
        "value-map-extend:is-map-value:extension-value-object");

    addPreTrigger(
        triggers,
        [] {
            const auto val = bt2::BoolValue::create();

            withCurrentThreadError([&] {
                bt_value *copyVal = nullptr;

                bt_value_copy(val->libObjPtr(), &copyVal);
            });
        },
        "value-copy:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_value *copyVal = nullptr;

            bt_value_copy(nullptr, &copyVal);
        },
        "value-copy:not-null:value-object");

    addPreTrigger(
        triggers,
        [] {
            bt_value_copy(bt2::BoolValue::create()->libObjPtr(), nullptr);
        },
        "value-copy:not-null:value-object-copy-output");

    addPreTrigger(
        triggers,
        [] {
            bt_value_is_equal(nullptr, bt2::BoolValue::create()->libObjPtr());
        },
        "value-is-equal:not-null:value-object-a");

    addPreTrigger(
        triggers,
        [] {
            bt_value_is_equal(bt2::BoolValue::create()->libObjPtr(), nullptr);
        },
        "value-is-equal:not-null:value-object-b");
}
