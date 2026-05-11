/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <array>

#include "cpp-common/bt2/integer-range-set.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Add triggers for field class vs. trace class match API.
 */
void addFcTcMatchTriggers(CondTriggers& triggers)
{
    /* Tests valid for all MIP versions */
    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            selfComp.createTraceClass()->createStaticArrayFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(), 10);
        },
        "field-class-array-static-create:fc-has-expected-trace-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            selfComp.createTraceClass()->createStructureFieldClass()->appendMember(
                "field", *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        "field-class-structure-append-member:fcs-have-same-trace-class", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            selfComp.createTraceClass()->createDynamicArrayFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        CondTrigger::Type::Pre, "field-class-array-dynamic-create:fc-has-expected-trace-class", 0,
        "elem-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls1 = selfComp.createTraceClass();

            traceCls1->createDynamicArrayFieldClass(
                *traceCls1->createUnsignedIntegerFieldClass(),
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        CondTrigger::Type::Pre, "field-class-array-dynamic-create:fc-has-expected-trace-class", 0,
        "len-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            selfComp.createTraceClass()->createOptionFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        CondTrigger::Type::Pre,
        "field-class-option-without-selector-create:fc-has-expected-trace-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();

            traceCls2->createOptionWithBoolSelectorFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(),
                *traceCls2->createBoolFieldClass());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-create:fc-has-expected-trace-class", 0,
        "opt-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls1 = selfComp.createTraceClass();

            traceCls1->createOptionWithBoolSelectorFieldClass(
                *traceCls1->createUnsignedIntegerFieldClass(),
                *selfComp.createTraceClass()->createBoolFieldClass());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-create:fc-has-expected-trace-class", 0,
        "sel-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls2->createOptionWithUnsignedIntegerSelectorFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(),
                *traceCls2->createUnsignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:fc-has-expected-trace-class",
        0, "opt-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls1 = selfComp.createTraceClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls1->createOptionWithUnsignedIntegerSelectorFieldClass(
                *traceCls1->createUnsignedIntegerFieldClass(),
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:fc-has-expected-trace-class",
        0, "sel-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls2->createOptionWithSignedIntegerSelectorFieldClass(
                *selfComp.createTraceClass()->createSignedIntegerFieldClass(),
                *traceCls2->createSignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:fc-has-expected-trace-class",
        0, "opt-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls1 = selfComp.createTraceClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls1->createOptionWithSignedIntegerSelectorFieldClass(
                *traceCls1->createSignedIntegerFieldClass(),
                *selfComp.createTraceClass()->createSignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:fc-has-expected-trace-class",
        0, "sel-different-tc"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            selfComp.createTraceClass()->createVariantWithUnsignedIntegerSelectorFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        CondTrigger::Type::Pre, "field-class-variant-create:fc-has-expected-trace-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            selfComp.createTraceClass()->createVariantFieldClass()->appendOption(
                "opt", *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-without-selector-append-option:fcs-have-same-trace-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls1 = selfComp.createTraceClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls1
                ->createVariantWithUnsignedIntegerSelectorFieldClass(
                    *traceCls1->createUnsignedIntegerFieldClass())
                ->appendOption("opt",
                               *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(),
                               *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:fcs-have-same-trace-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls1 = selfComp.createTraceClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls1
                ->createVariantWithSignedIntegerSelectorFieldClass(
                    *traceCls1->createSignedIntegerFieldClass())
                ->appendOption("opt", *selfComp.createTraceClass()->createSignedIntegerFieldClass(),
                               *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:fcs-have-same-trace-class",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            selfComp.createTraceClass()->createDynamicArrayWithoutLengthFieldLocationFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        "field-class-array-dynamic-without-length-field-location-create:fc-has-expected-trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            selfComp.createTraceClass()->createOptionWithoutSelectorFieldLocationFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass());
        },
        "field-class-option-without-selector-field-location-create:fc-has-expected-trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();

            traceCls2->createOptionWithBoolSelectorFieldLocationFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(),
                *traceCls2->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                                std::array<const char *, 1> {"len"}));
        },
        "field-class-option-with-selector-field-location-bool-create:fc-has-expected-trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls2->createOptionWithUnsignedIntegerSelectorFieldLocationFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(),
                *traceCls2->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                                std::array<const char *, 1> {"sel"}),
                *ranges);
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:fc-has-expected-trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(1, 10);
            traceCls2->createOptionWithSignedIntegerSelectorFieldLocationFieldClass(
                *selfComp.createTraceClass()->createSignedIntegerFieldClass(),
                *traceCls2->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                                std::array<const char *, 1> {"sel"}),
                *ranges);
        },
        "field-class-option-with-selector-field-location-integer-signed-create:fc-has-expected-trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls2 = selfComp.createTraceClass();

            traceCls2->createDynamicArrayWithLengthFieldLocationFieldClass(
                *selfComp.createTraceClass()->createUnsignedIntegerFieldClass(),
                *traceCls2->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                                std::array<const char *, 1> {"len"}));
        },
        "field-class-array-dynamic-with-length-field-location-create:fc-has-expected-trace-class");
}
