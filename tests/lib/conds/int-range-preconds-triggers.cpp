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
 * Adds integer range API precondition failure triggers.
 */
void addIntRangePreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_unsigned_get_lower(nullptr);
        },
        "integer-range-unsigned-get-lower:not-null:integer-range");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_unsigned_get_upper(nullptr);
        },
        "integer-range-unsigned-get-upper:not-null:integer-range");

    addPreTrigger(
        triggers,
        [] {
            const auto rangeSet = bt2::UnsignedIntegerRangeSet::create();

            rangeSet->addRange(0, 0);
            bt_integer_range_unsigned_is_equal(nullptr, (*rangeSet)[0].libObjPtr());
        },
        "integer-range-unsigned-is-equal:not-null:integer-range-a");

    addPreTrigger(
        triggers,
        [] {
            const auto rangeSet = bt2::UnsignedIntegerRangeSet::create();

            rangeSet->addRange(0, 0);
            bt_integer_range_unsigned_is_equal((*rangeSet)[0].libObjPtr(), nullptr);
        },
        "integer-range-unsigned-is-equal:not-null:integer-range-b");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_signed_get_lower(nullptr);
        },
        "integer-range-signed-get-lower:not-null:integer-range");

    addPreTrigger(
        triggers,
        [] {
            bt_integer_range_signed_get_upper(nullptr);
        },
        "integer-range-signed-get-upper:not-null:integer-range");

    addPreTrigger(
        triggers,
        [] {
            const auto rangeSet = bt2::SignedIntegerRangeSet::create();

            rangeSet->addRange(0, 0);
            bt_integer_range_signed_is_equal(nullptr, (*rangeSet)[0].libObjPtr());
        },
        "integer-range-signed-is-equal:not-null:integer-range-a");

    addPreTrigger(
        triggers,
        [] {
            const auto rangeSet = bt2::SignedIntegerRangeSet::create();

            rangeSet->addRange(0, 0);
            bt_integer_range_signed_is_equal((*rangeSet)[0].libObjPtr(), nullptr);
        },
        "integer-range-signed-is-equal:not-null:integer-range-b");
}
