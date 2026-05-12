/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/integer-range-set.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Adds integer range set API precondition failure triggers.
 */
void addIntRangeSetPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_get_range_count(nullptr);
        },
        "integer-range-set-get-range-count:not-null:integer-range-set");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_create();
        },
        "integer-range-set-unsigned-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            const auto rangeSet = bt2::UnsignedIntegerRangeSet::create();

            withCurrentThreadError([&] {
                bt_integer_range_set_unsigned_add_range(rangeSet->libObjPtr(), 0, 0);
            });
        },
        "integer-range-set-unsigned-add-range:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_add_range(nullptr, 0, 0);
        },
        "integer-range-set-unsigned-add-range:not-null:integer-range-set");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const auto rangeSet = bt2::UnsignedIntegerRangeSet::create();

            rangeSet->addRange(0, 0);
            traceCls->createOptionWithUnsignedIntegerSelectorFieldClass(
                *contentFc, *traceCls->createUnsignedIntegerFieldClass(), *rangeSet);
            bt_integer_range_set_unsigned_add_range(rangeSet->libObjPtr(), 1, 1);
        },
        CondTrigger::Type::Pre, "integer-range-set-unsigned-add-range:not-frozen:integer-range-set",
        0));

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_add_range(
                bt2::UnsignedIntegerRangeSet::create()->libObjPtr(), 5, 1);
        },
        "integer-range-set-unsigned-add-range:lower-lteq-upper");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_borrow_range_by_index_const(nullptr, 0);
        },
        "integer-range-set-unsigned-borrow-range-by-index-const:not-null:integer-range-set");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_borrow_range_by_index_const(
                bt2::UnsignedIntegerRangeSet::create()->libObjPtr(), 0);
        },
        "integer-range-set-unsigned-borrow-range-by-index-const:valid-index");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_is_equal(
                nullptr, bt2::UnsignedIntegerRangeSet::create()->libObjPtr());
        },
        "integer-range-set-unsigned-is-equal:not-null:integer-range-set-a");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_unsigned_is_equal(
                bt2::UnsignedIntegerRangeSet::create()->libObjPtr(), nullptr);
        },
        "integer-range-set-unsigned-is-equal:not-null:integer-range-set-b");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_create();
        },
        "integer-range-set-signed-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            const auto rangeSet = bt2::SignedIntegerRangeSet::create();

            withCurrentThreadError([&] {
                bt_integer_range_set_signed_add_range(rangeSet->libObjPtr(), 0, 0);
            });
        },
        "integer-range-set-signed-add-range:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_add_range(nullptr, 0, 0);
        },
        "integer-range-set-signed-add-range:not-null:integer-range-set");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const auto rangeSet = bt2::SignedIntegerRangeSet::create();

            rangeSet->addRange(0, 0);
            traceCls->createOptionWithSignedIntegerSelectorFieldClass(
                *contentFc, *traceCls->createSignedIntegerFieldClass(), *rangeSet);
            bt_integer_range_set_signed_add_range(rangeSet->libObjPtr(), 1, 1);
        },
        CondTrigger::Type::Pre, "integer-range-set-signed-add-range:not-frozen:integer-range-set",
        0));

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_add_range(bt2::SignedIntegerRangeSet::create()->libObjPtr(),
                                                  5, 1);
        },
        "integer-range-set-signed-add-range:lower-lteq-upper");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_borrow_range_by_index_const(nullptr, 0);
        },
        "integer-range-set-signed-borrow-range-by-index-const:not-null:integer-range-set");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_borrow_range_by_index_const(
                bt2::SignedIntegerRangeSet::create()->libObjPtr(), 0);
        },
        "integer-range-set-signed-borrow-range-by-index-const:valid-index");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_is_equal(nullptr,
                                                 bt2::SignedIntegerRangeSet::create()->libObjPtr());
        },
        "integer-range-set-signed-is-equal:not-null:integer-range-set-a");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_set_signed_is_equal(bt2::SignedIntegerRangeSet::create()->libObjPtr(),
                                                 nullptr);
        },
        "integer-range-set-signed-is-equal:not-null:integer-range-set-b");
}
