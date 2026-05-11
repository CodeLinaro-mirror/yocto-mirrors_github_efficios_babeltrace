/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <cstdint>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/integer-range-set.hpp"
#include "cpp-common/bt2/mip.hpp"
#include "cpp-common/bt2s/span.hpp"
#include "cpp-common/vendor/fmt/core.h"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

/*
 * Returns a freshly-created MIP 0 unsigned integer field class.
 */
bt2::IntegerFieldClass::Shared createUIntFc(const bt2::SelfComponent selfComp)
{
    return selfComp.createTraceClass()->createUnsignedIntegerFieldClass();
}

/*
 * Returns a frozen unsigned integer field class (created from the trace
 * class of `selfComp`).
 *
 * The field class is frozen by being added as a member of a structure
 * field class which is itself nested in another structure field class.
 */
bt2::IntegerFieldClass::Shared createFrozenUIntFc(const bt2::SelfComponent selfComp)
{
    const auto traceCls = selfComp.createTraceClass();
    const auto intFc = traceCls->createUnsignedIntegerFieldClass();

    /*
     * Adding the integer field class as a member of a structure field
     * class freezes the integer field class.
     *
     * The temporary structure field class is released at the end of the
     * statement (no leak), but the integer field class survives through
     * `intFc` and stays frozen.
     */
    traceCls->createStructureFieldClass()->appendMember("m", *intFc);
    return intFc;
}

} /* namespace */

/*
 * Adds field class API precondition failure triggers.
 */
void addFcPreCondsTriggers(CondTriggers& triggers)
{
    const std::uint64_t maxMip = bt2::getMaximalMipVersion();

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_get_type(nullptr);
        },
        "field-class-get-type:not-null:field-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_create(nullptr, 1);
        },
        "field-class-bit-array-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_create(nullptr, 1);
        },
        "field-class-bit-array-create:not-null:trace-class");

    for (std::uint64_t mipVersion = 0; mipVersion <= maxMip; ++mipVersion) {
        triggers.emplace_back(makeRunInCompInitTrigger(
            [](const auto selfComp) {
                selfComp.createTraceClass()->createBitArrayFieldClass(0);
            },
            CondTrigger::Type::Pre, "field-class-bit-array-create:valid-length", mipVersion,
            fmt::format("mip{}-0", mipVersion)));

        triggers.emplace_back(makeRunInCompInitTrigger(
            [](const auto selfComp) {
                selfComp.createTraceClass()->createBitArrayFieldClass(65);
            },
            CondTrigger::Type::Pre, "field-class-bit-array-create:valid-length", mipVersion,
            fmt::format("mip{}-gt-64", mipVersion)));
    }

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_get_length(nullptr);
        },
        "field-class-bit-array-get-length:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_get_length(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-bit-array-get-length:is-bit-array:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_get_flag_count(nullptr);
        },
        "field-class-bit-array-get-flag-count:not-null:field-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_bit_array_get_flag_count(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr());
        },
        CondTrigger::Type::Pre, "field-class-bit-array-get-flag-count:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_get_flag_count(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-bit-array-get-flag-count:is-bit-array:field-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_add_flag(nullptr, "x", nullptr);
        },
        "field-class-bit-array-add-flag:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_add_flag(nullptr, "x", nullptr);
        },
        "field-class-bit-array-add-flag:not-null:field-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_bit_array_add_flag(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), "f",
                nullptr);
        },
        CondTrigger::Type::Pre, "field-class-bit-array-add-flag:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_add_flag(createUIntFc(selfComp)->libObjPtr(), "f", nullptr);
        },
        "field-class-bit-array-add-flag:is-bit-array:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_add_flag(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), nullptr,
                nullptr);
        },
        "field-class-bit-array-add-flag:not-null:label");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto fc = selfComp.createTraceClass()->createBitArrayFieldClass(8);
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            fc->addFlag("dup", *ranges);
            fc->addFlag("dup", *ranges);
        },
        "field-class-bit-array-add-flag:bit-array-field-class-flag-label-is-unique");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_add_flag(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), "f",
                nullptr);
        },
        "field-class-bit-array-add-flag:not-null:integer-range-set");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            /* Upper bound 4 is >= length 4 */
            ranges->addRange(4, 4);
            selfComp.createTraceClass()->createBitArrayFieldClass(4)->addFlag("f", *ranges);
        },
        "field-class-bit-array-add-flag:bit-array-field-class-flag-bit-index-is-less-than-field-class-length");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_borrow_flag_by_index_const(nullptr, 0);
        },
        "field-class-bit-array-borrow-flag-by-index-const:not-null:field-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_bit_array_borrow_flag_by_index_const(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), 0);
        },
        CondTrigger::Type::Pre,
        "field-class-bit-array-borrow-flag-by-index-const:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_borrow_flag_by_index_const(createUIntFc(selfComp)->libObjPtr(),
                                                                0);
        },
        "field-class-bit-array-borrow-flag-by-index-const:is-bit-array:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            /* No flags added: index 0 is out of bounds. */
            bt_field_class_bit_array_borrow_flag_by_index_const(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), 0);
        },
        "field-class-bit-array-borrow-flag-by-index-const:valid-index");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_borrow_flag_by_label_const(nullptr, "x");
        },
        "field-class-bit-array-borrow-flag-by-label-const:not-null:field-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_bit_array_borrow_flag_by_label_const(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), "x");
        },
        CondTrigger::Type::Pre,
        "field-class-bit-array-borrow-flag-by-label-const:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_borrow_flag_by_label_const(createUIntFc(selfComp)->libObjPtr(),
                                                                "x");
        },
        "field-class-bit-array-borrow-flag-by-label-const:is-bit-array:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_borrow_flag_by_label_const(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), nullptr);
        },
        "field-class-bit-array-borrow-flag-by-label-const:not-null:label");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_get_active_flag_labels_for_value_as_integer(nullptr, 0,
                                                                                 nullptr, nullptr);
        },
        "field-class-bit-array-get-active-flag-labels-for-value-as-integer:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_get_active_flag_labels_for_value_as_integer(nullptr, 0,
                                                                                 nullptr, nullptr);
        },
        "field-class-bit-array-get-active-flag-labels-for-value-as-integer:not-null:field-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_bit_array_flag_label_array labels;
            std::uint64_t count;

            bt_field_class_bit_array_get_active_flag_labels_for_value_as_integer(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), 0, &labels,
                &count);
        },
        CondTrigger::Type::Pre,
        "field-class-bit-array-get-active-flag-labels-for-value-as-integer:mip-version-is-valid",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_flag_label_array labels;
            std::uint64_t count;

            bt_field_class_bit_array_get_active_flag_labels_for_value_as_integer(
                createUIntFc(selfComp)->libObjPtr(), 0, &labels, &count);
        },
        "field-class-bit-array-get-active-flag-labels-for-value-as-integer:is-bit-array:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            std::uint64_t count;

            bt_field_class_bit_array_get_active_flag_labels_for_value_as_integer(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), 0, nullptr,
                &count);
        },
        "field-class-bit-array-get-active-flag-labels-for-value-as-integer:not-null:label-array-output");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_bit_array_flag_label_array labels;

            bt_field_class_bit_array_get_active_flag_labels_for_value_as_integer(
                selfComp.createTraceClass()->createBitArrayFieldClass(8)->libObjPtr(), 0, &labels,
                nullptr);
        },
        "field-class-bit-array-get-active-flag-labels-for-value-as-integer:not-null:count-output");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_flag_get_label(nullptr);
        },
        "field-class-bit-array-flag-get-label:not-null:bit-array-field-class-flag");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bit_array_flag_borrow_index_ranges_const(nullptr);
        },
        "field-class-bit-array-flag-borrow-index-ranges-const:not-null:bit-array-field-class-flag");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_bool_create(nullptr);
        },
        "field-class-bool-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_bool_create(nullptr);
        },
        "field-class-bool-create:not-null:trace-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_integer_unsigned_create(nullptr);
        },
        "field-class-integer-unsigned-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_integer_signed_create(nullptr);
        },
        "field-class-integer-signed-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_integer_get_field_value_range(nullptr);
        },
        "field-class-integer-get-field-value-range:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_get_field_value_range(
                selfComp.createTraceClass()->createBoolFieldClass()->libObjPtr());
        },
        "field-class-integer-get-field-value-range:is-int-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_integer_set_field_value_range(nullptr, 23);
        },
        "field-class-integer-set-field-value-range:not-null:field-class");

    for (std::uint64_t mipVersion = 0; mipVersion <= maxMip; ++mipVersion) {
        triggers.emplace_back(makeRunInCompInitTrigger(
            [](const auto selfComp) {
                createUIntFc(selfComp)->fieldValueRange(0);
            },
            CondTrigger::Type::Pre, "field-class-integer-set-field-value-range:valid-n", mipVersion,
            fmt::format("mip{}-0", mipVersion)));

        triggers.emplace_back(makeRunInCompInitTrigger(
            [](const auto selfComp) {
                createUIntFc(selfComp)->fieldValueRange(65);
            },
            CondTrigger::Type::Pre, "field-class-integer-set-field-value-range:valid-n", mipVersion,
            fmt::format("mip{}-gt-64", mipVersion)));
    }

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_field_value_range(
                selfComp.createTraceClass()->createBoolFieldClass()->libObjPtr(), 32);
        },
        "field-class-integer-set-field-value-range:is-int-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_field_value_range(createFrozenUIntFc(selfComp)->libObjPtr(),
                                                         32);
        },
        "field-class-integer-set-field-value-range:not-frozen:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_integer_set_field_value_hints(nullptr, 0);
        },
        "field-class-integer-set-field-value-hints:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_field_value_hints(
                selfComp.createTraceClass()->createBoolFieldClass()->libObjPtr(), 0);
        },
        "field-class-integer-set-field-value-hints:is-int-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_field_value_hints(createFrozenUIntFc(selfComp)->libObjPtr(),
                                                         0);
        },
        "field-class-integer-set-field-value-hints:not-frozen:field-class", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_integer_set_field_value_hints(createUIntFc(selfComp)->libObjPtr(), 0);
        },
        CondTrigger::Type::Pre, "field-class-integer-set-field-value-hints:mip-version-is-valid",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_field_value_hints(createUIntFc(selfComp)->libObjPtr(), 0xff);
        },
        "field-class-integer-set-field-value-hints:hint-exists");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_integer_get_field_value_hints(nullptr);
        },
        "field-class-integer-get-field-value-hints:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_get_field_value_hints(
                selfComp.createTraceClass()->createBoolFieldClass()->libObjPtr());
        },
        "field-class-integer-get-field-value-hints:is-int-field-class:field-class", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_integer_get_field_value_hints(createUIntFc(selfComp)->libObjPtr());
        },
        CondTrigger::Type::Pre, "field-class-integer-get-field-value-hints:mip-version-is-valid",
        0));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_integer_get_preferred_display_base(nullptr);
        },
        "field-class-integer-get-preferred-display-base:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_get_preferred_display_base(
                selfComp.createTraceClass()->createBoolFieldClass()->libObjPtr());
        },
        "field-class-integer-get-preferred-display-base:is-int-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_integer_set_preferred_display_base(
                nullptr, BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_DECIMAL);
        },
        "field-class-integer-set-preferred-display-base:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_preferred_display_base(
                selfComp.createTraceClass()->createBoolFieldClass()->libObjPtr(),
                BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_DECIMAL);
        },
        "field-class-integer-set-preferred-display-base:is-int-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_integer_set_preferred_display_base(
                createFrozenUIntFc(selfComp)->libObjPtr(),
                BT_FIELD_CLASS_INTEGER_PREFERRED_DISPLAY_BASE_DECIMAL);
        },
        "field-class-integer-set-preferred-display-base:not-frozen:field-class", 0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_create(nullptr);
        },
        "field-class-enumeration-unsigned-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_create(nullptr);
        },
        "field-class-enumeration-signed-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_get_mapping_count(nullptr);
        },
        "field-class-enumeration-get-mapping-count:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_get_mapping_count(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-enumeration-get-mapping-count:is-enumeration-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_borrow_mapping_by_index_const(nullptr, 0);
        },
        "field-class-enumeration-unsigned-borrow-mapping-by-index-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_unsigned_borrow_mapping_by_index_const(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(), 0);
        },
        "field-class-enumeration-unsigned-borrow-mapping-by-index-const:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto fc = selfComp.createTraceClass()->createSignedEnumerationFieldClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            fc->addMapping("m", *ranges);
            bt_field_class_enumeration_unsigned_borrow_mapping_by_index_const(fc->libObjPtr(), 0);
        },
        "field-class-enumeration-unsigned-borrow-mapping-by-index-const:is-unsigned-enumeration:field-class",
        0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_borrow_mapping_by_index_const(nullptr, 0);
        },
        "field-class-enumeration-signed-borrow-mapping-by-index-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_signed_borrow_mapping_by_index_const(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(), 0);
        },
        "field-class-enumeration-signed-borrow-mapping-by-index-const:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto fc = selfComp.createTraceClass()->createUnsignedEnumerationFieldClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            fc->addMapping("m", *ranges);
            bt_field_class_enumeration_signed_borrow_mapping_by_index_const(fc->libObjPtr(), 0);
        },
        "field-class-enumeration-signed-borrow-mapping-by-index-const:is-signed-enumeration:field-class",
        0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_borrow_mapping_by_label_const(nullptr, "x");
        },
        "field-class-enumeration-signed-borrow-mapping-by-label-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_signed_borrow_mapping_by_label_const(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(),
                "x");
        },
        "field-class-enumeration-signed-borrow-mapping-by-label-const:is-signed-enumeration:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_signed_borrow_mapping_by_label_const(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(),
                nullptr);
        },
        "field-class-enumeration-signed-borrow-mapping-by-label-const:not-null:label", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_borrow_mapping_by_label_const(nullptr, "x");
        },
        "field-class-enumeration-unsigned-borrow-mapping-by-label-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_unsigned_borrow_mapping_by_label_const(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(), "x");
        },
        "field-class-enumeration-unsigned-borrow-mapping-by-label-const:is-unsigned-enumeration:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_unsigned_borrow_mapping_by_label_const(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(),
                nullptr);
        },
        "field-class-enumeration-unsigned-borrow-mapping-by-label-const:not-null:label", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_mapping_get_label(nullptr);
        },
        "field-class-enumeration-mapping-get-label:not-null:enumeration-field-class-mapping");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_mapping_borrow_ranges_const(nullptr);
        },
        "field-class-enumeration-unsigned-mapping-borrow-ranges-const:not-null:enumeration-field-class-mapping");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_mapping_borrow_ranges_const(nullptr);
        },
        "field-class-enumeration-signed-mapping-borrow-ranges-const:not-null:enumeration-field-class-mapping");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_get_mapping_labels_for_value(nullptr, 0, nullptr,
                                                                             nullptr);
        },
        "field-class-enumeration-unsigned-get-mapping-labels-for-value:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_get_mapping_labels_for_value(nullptr, 0, nullptr,
                                                                             nullptr);
        },
        "field-class-enumeration-unsigned-get-mapping-labels-for-value:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            std::uint64_t count;

            bt_field_class_enumeration_unsigned_get_mapping_labels_for_value(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(), 0,
                nullptr, &count);
        },
        "field-class-enumeration-unsigned-get-mapping-labels-for-value:not-null:label-array-output",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_mapping_label_array labels;

            bt_field_class_enumeration_unsigned_get_mapping_labels_for_value(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(), 0,
                &labels, nullptr);
        },
        "field-class-enumeration-unsigned-get-mapping-labels-for-value:not-null:count-output", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_mapping_label_array labels;
            std::uint64_t count;

            bt_field_class_enumeration_unsigned_get_mapping_labels_for_value(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(), 0,
                &labels, &count);
        },
        "field-class-enumeration-unsigned-get-mapping-labels-for-value:is-unsigned-enumeration:field-class",
        0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_get_mapping_labels_for_value(nullptr, 0, nullptr,
                                                                           nullptr);
        },
        "field-class-enumeration-signed-get-mapping-labels-for-value:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_get_mapping_labels_for_value(nullptr, 0, nullptr,
                                                                           nullptr);
        },
        "field-class-enumeration-signed-get-mapping-labels-for-value:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            std::uint64_t count;

            bt_field_class_enumeration_signed_get_mapping_labels_for_value(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(), 0,
                nullptr, &count);
        },
        "field-class-enumeration-signed-get-mapping-labels-for-value:not-null:label-array-output",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_mapping_label_array labels;

            bt_field_class_enumeration_signed_get_mapping_labels_for_value(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(), 0,
                &labels, nullptr);
        },
        "field-class-enumeration-signed-get-mapping-labels-for-value:not-null:count-output", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_mapping_label_array labels;
            std::uint64_t count;

            bt_field_class_enumeration_signed_get_mapping_labels_for_value(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(), 0,
                &labels, &count);
        },
        "field-class-enumeration-signed-get-mapping-labels-for-value:is-signed-enumeration:field-class",
        0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_add_mapping(nullptr, "x", nullptr);
        },
        "field-class-enumeration-unsigned-add-mapping:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_unsigned_add_mapping(nullptr, "x", nullptr);
        },
        "field-class-enumeration-unsigned-add-mapping:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_unsigned_add_mapping(createUIntFc(selfComp)->libObjPtr(),
                                                            "x", nullptr);
        },
        "field-class-enumeration-unsigned-add-mapping:is-unsigned-enumeration-field-class:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_unsigned_add_mapping(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(),
                nullptr, nullptr);
        },
        "field-class-enumeration-unsigned-add-mapping:not-null:label", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_unsigned_add_mapping(
                selfComp.createTraceClass()->createUnsignedEnumerationFieldClass()->libObjPtr(),
                "x", nullptr);
        },
        "field-class-enumeration-unsigned-add-mapping:not-null:integer-range-set", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto fc = selfComp.createTraceClass()->createUnsignedEnumerationFieldClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            fc->addMapping("dup", *ranges);
            fc->addMapping("dup", *ranges);
        },
        "field-class-enumeration-unsigned-add-mapping:enumeration-field-class-mapping-label-is-unique",
        0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_add_mapping(nullptr, "x", nullptr);
        },
        "field-class-enumeration-signed-add-mapping:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_enumeration_signed_add_mapping(nullptr, "x", nullptr);
        },
        "field-class-enumeration-signed-add-mapping:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_signed_add_mapping(createUIntFc(selfComp)->libObjPtr(), "x",
                                                          nullptr);
        },
        "field-class-enumeration-signed-add-mapping:is-signed-enumeration-field-class:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_signed_add_mapping(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(),
                nullptr, nullptr);
        },
        "field-class-enumeration-signed-add-mapping:not-null:label", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_enumeration_signed_add_mapping(
                selfComp.createTraceClass()->createSignedEnumerationFieldClass()->libObjPtr(), "x",
                nullptr);
        },
        "field-class-enumeration-signed-add-mapping:not-null:integer-range-set", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto fc = selfComp.createTraceClass()->createSignedEnumerationFieldClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            fc->addMapping("dup", *ranges);
            fc->addMapping("dup", *ranges);
        },
        "field-class-enumeration-signed-add-mapping:enumeration-field-class-mapping-label-is-unique",
        0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_real_single_precision_create(nullptr);
        },
        "field-class-real-single-precision-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_real_double_precision_create(nullptr);
        },
        "field-class-real-double-precision-create:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_structure_create(nullptr);
        },
        "field-class-structure-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_create(nullptr);
        },
        "field-class-structure-create:not-null:trace-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_structure_append_member(nullptr, "x", nullptr);
        },
        "field-class-structure-append-member:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_append_member(nullptr, "x", nullptr);
        },
        "field-class-structure-append-member:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_append_member(createUIntFc(selfComp)->libObjPtr(), "x",
                                                   nullptr);
        },
        "field-class-structure-append-member:is-structure-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_append_member(
                selfComp.createTraceClass()->createStructureFieldClass()->libObjPtr(), nullptr,
                nullptr);
        },
        "field-class-structure-append-member:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_append_member(
                selfComp.createTraceClass()->createStructureFieldClass()->libObjPtr(), "x",
                nullptr);
        },
        "field-class-structure-append-member:not-null:member-field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto structFc = traceCls->createStructureFieldClass();

            traceCls->createStructureFieldClass()->appendMember("inner", *structFc);
            structFc->appendMember("m", *traceCls->createUnsignedIntegerFieldClass());
        },
        "field-class-structure-append-member:not-frozen:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("dup", *traceCls->createUnsignedIntegerFieldClass());
            structFc->appendMember("dup", *traceCls->createUnsignedIntegerFieldClass());
        },
        "field-class-structure-append-member:structure-field-class-member-name-is-unique", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto outerStructFc = traceCls->createStructureFieldClass();
            const auto memberFc = traceCls->createUnsignedIntegerFieldClass();

            outerStructFc->appendMember("first", *memberFc);
            outerStructFc->appendMember("second", *memberFc);
        },
        "field-class-structure-append-member:field-class-is-not-part-of-something", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_get_member_count(nullptr);
        },
        "field-class-structure-get-member-count:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_get_member_count(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-structure-get-member-count:is-structure-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_borrow_member_by_index(nullptr, 0);
        },
        "field-class-structure-borrow-member-by-index:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_borrow_member_by_index_const(nullptr, 0);
        },
        "field-class-structure-borrow-member-by-index-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_index(createUIntFc(selfComp)->libObjPtr(), 0);
        },
        "field-class-structure-borrow-member-by-index:is-structure-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_index_const(
                createUIntFc(selfComp)->libObjPtr(), 0);
        },
        "field-class-structure-borrow-member-by-index-const:is-structure-field-class:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_index(
                selfComp.createTraceClass()->createStructureFieldClass()->libObjPtr(), 0);
        },
        "field-class-structure-borrow-member-by-index:valid-index", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_index_const(
                selfComp.createTraceClass()->createStructureFieldClass()->libObjPtr(), 0);
        },
        "field-class-structure-borrow-member-by-index-const:valid-index", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_borrow_member_by_name(nullptr, "x");
        },
        "field-class-structure-borrow-member-by-name:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_borrow_member_by_name_const(nullptr, "x");
        },
        "field-class-structure-borrow-member-by-name-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_name(createUIntFc(selfComp)->libObjPtr(),
                                                           "x");
        },
        "field-class-structure-borrow-member-by-name:is-structure-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_name_const(
                createUIntFc(selfComp)->libObjPtr(), "x");
        },
        "field-class-structure-borrow-member-by-name-const:is-structure-field-class:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_name(
                selfComp.createTraceClass()->createStructureFieldClass()->libObjPtr(), nullptr);
        },
        "field-class-structure-borrow-member-by-name:not-null:name", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_structure_borrow_member_by_name_const(
                selfComp.createTraceClass()->createStructureFieldClass()->libObjPtr(), nullptr);
        },
        "field-class-structure-borrow-member-by-name-const:not-null:name", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_member_get_name(nullptr);
        },
        "field-class-structure-member-get-name:not-null:structure-field-class-member");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_member_borrow_field_class(nullptr);
        },
        "field-class-structure-member-borrow-field-class:not-null:structure-field-class-member");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_member_borrow_field_class_const(nullptr);
        },
        "field-class-structure-member-borrow-field-class-const:not-null:structure-field-class-member");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_without_selector_create(nullptr, nullptr);
        },
        "field-class-option-without-selector-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_without_selector_create(nullptr, nullptr);
        },
        "field-class-option-without-selector-create:not-null:trace-class");

    /* `mip-version-is-valid`: this MIP-0-only API in MIP 1+ */
    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_without_selector_create(selfComp.createTraceClass()->libObjPtr(),
                                                          nullptr);
        },
        "field-class-option-without-selector-create:mip-version-is-valid");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_without_selector_create(selfComp.createTraceClass()->libObjPtr(),
                                                          nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-without-selector-create:not-null:content-field-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *contentFc);
            traceCls->createOptionFieldClass(*contentFc);
        },
        CondTrigger::Type::Pre,
        "field-class-option-without-selector-create:field-class-is-not-part-of-something", 0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_without_selector_field_location_create(nullptr, nullptr);
        },
        "field-class-option-without-selector-field-location-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_without_selector_field_location_create(nullptr, nullptr);
        },
        "field-class-option-without-selector-field-location-create:not-null:trace-class");

    /* `mip-version-is-valid`: this MIP-1+ API in MIP 0 */
    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_without_selector_field_location_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-without-selector-field-location-create:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_without_selector_field_location_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        "field-class-option-without-selector-field-location-create:not-null:content-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithoutSelectorFieldLocationFieldClass(*contentFc);
        },
        "field-class-option-without-selector-field-location-create:field-class-is-not-part-of-something");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_bool_create(nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-bool-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_bool_create(nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-bool-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_bool_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr);
        },
        "field-class-option-with-selector-field-bool-create:mip-version-is-valid");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_bool_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-create:not-null:selector-field-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_bool_create(
                traceCls->libObjPtr(), traceCls->createUnsignedIntegerFieldClass()->libObjPtr(),
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-create:is-boolean-field-class:selector-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_bool_create(
                traceCls->libObjPtr(), nullptr, traceCls->createBoolFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-create:not-null:content-field-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithBoolSelectorFieldClass(*contentFc,
                                                             *traceCls->createBoolFieldClass());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-create:field-class-is-not-part-of-something",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_location_bool_create(nullptr, nullptr,
                                                                           nullptr);
        },
        "field-class-option-with-selector-field-location-bool-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_location_bool_create(nullptr, nullptr,
                                                                           nullptr);
        },
        "field-class-option-with-selector-field-location-bool-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_location_bool_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-location-bool-create:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_location_bool_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-bool-create:not-null:selector-field-location");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_option_with_selector_field_location_bool_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr());
        },
        "field-class-option-with-selector-field-location-bool-create:not-null:content-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithBoolSelectorFieldLocationFieldClass(*contentFc, *fieldLoc);
        },
        "field-class-option-with-selector-field-location-bool-create:field-class-is-not-part-of-something");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_integer_unsigned_create(nullptr, nullptr,
                                                                              nullptr, nullptr);
        },
        "field-class-option-with-selector-field-integer-unsigned-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_integer_unsigned_create(nullptr, nullptr,
                                                                              nullptr, nullptr);
        },
        "field-class-option-with-selector-field-integer-unsigned-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_integer_unsigned_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-integer-unsigned-create:mip-version-is-valid");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_integer_unsigned_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:not-null:selector-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createSignedIntegerFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:is-unsigned-integer-field-class:selector-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:not-null:integer-range-set",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr(),
                bt2::UnsignedIntegerRangeSet::create()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:integer-range-set-is-not-empty",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            bt_field_class_option_with_selector_field_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr(), ranges->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:not-null:content-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();
            const auto structFc = traceCls->createStructureFieldClass();

            ranges->addRange(0, 0);
            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithUnsignedIntegerSelectorFieldClass(
                *contentFc, *traceCls->createUnsignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-unsigned-create:field-class-is-not-part-of-something",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                nullptr, nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                nullptr, nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-location-integer-unsigned-create:mip-version-is-valid",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:not-null:selector-field-location");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr(), nullptr);
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:not-null:integer-range-set");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr(), ranges->libObjPtr());
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:integer-range-set-is-not-empty");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            bt_field_class_option_with_selector_field_location_integer_unsigned_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr(), ranges->libObjPtr());
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:not-null:content-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();
            const auto structFc = traceCls->createStructureFieldClass();

            ranges->addRange(0, 0);
            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithUnsignedIntegerSelectorFieldLocationFieldClass(
                *contentFc, *fieldLoc, *ranges);
        },
        "field-class-option-with-selector-field-location-integer-unsigned-create:field-class-is-not-part-of-something");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_integer_signed_create(nullptr, nullptr,
                                                                            nullptr, nullptr);
        },
        "field-class-option-with-selector-field-integer-signed-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_integer_signed_create(nullptr, nullptr,
                                                                            nullptr, nullptr);
        },
        "field-class-option-with-selector-field-integer-signed-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_integer_signed_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-integer-signed-create:mip-version-is-valid");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_integer_signed_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:not-null:selector-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_integer_signed_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:is-signed-integer-field-class:selector-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_integer_signed_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createSignedIntegerFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:not-null:integer-range-set",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_integer_signed_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createSignedIntegerFieldClass()->libObjPtr(),
                bt2::SignedIntegerRangeSet::create()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:integer-range-set-is-not-empty",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            bt_field_class_option_with_selector_field_integer_signed_create(
                traceCls->libObjPtr(), nullptr,
                traceCls->createSignedIntegerFieldClass()->libObjPtr(), ranges->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:not-null:content-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createSignedIntegerFieldClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();
            const auto structFc = traceCls->createStructureFieldClass();

            ranges->addRange(0, 0);
            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithSignedIntegerSelectorFieldClass(
                *contentFc, *traceCls->createSignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-integer-signed-create:field-class-is-not-part-of-something",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_location_integer_signed_create(
                nullptr, nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-integer-signed-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_location_integer_signed_create(
                nullptr, nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-integer-signed-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_location_integer_signed_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-location-integer-signed-create:mip-version-is-valid",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_location_integer_signed_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr, nullptr);
        },
        "field-class-option-with-selector-field-location-integer-signed-create:not-null:selector-field-location");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_option_with_selector_field_location_integer_signed_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr(), nullptr);
        },
        "field-class-option-with-selector-field-location-integer-signed-create:not-null:integer-range-set");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            bt_field_class_option_with_selector_field_location_integer_signed_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr(), ranges->libObjPtr());
        },
        "field-class-option-with-selector-field-location-integer-signed-create:integer-range-set-is-not-empty");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            bt_field_class_option_with_selector_field_location_integer_signed_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr(), ranges->libObjPtr());
        },
        "field-class-option-with-selector-field-location-integer-signed-create:not-null:content-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createSignedIntegerFieldClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto ranges = bt2::SignedIntegerRangeSet::create();
            const auto structFc = traceCls->createStructureFieldClass();

            ranges->addRange(0, 0);
            structFc->appendMember("m", *contentFc);
            traceCls->createOptionWithSignedIntegerSelectorFieldLocationFieldClass(
                *contentFc, *fieldLoc, *ranges);
        },
        "field-class-option-with-selector-field-location-integer-signed-create:field-class-is-not-part-of-something");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_borrow_field_class(nullptr);
        },
        "field-class-option-borrow-field-class:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_borrow_field_class_const(nullptr);
        },
        "field-class-option-borrow-field-class-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_borrow_field_class(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-borrow-field-class:is-option-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_borrow_field_class_const(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-borrow-field-class-const:is-option-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_borrow_selector_field_path_const(nullptr);
        },
        "field-class-option-with-selector-field-borrow-selector-field-path-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_borrow_selector_field_path_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-with-selector-field-borrow-selector-field-path-const:is-option-field-class-with-selector:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto contentFc = traceCls->createUnsignedIntegerFieldClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_option_with_selector_field_borrow_selector_field_path_const(
                traceCls->createOptionWithBoolSelectorFieldLocationFieldClass(*contentFc, *fieldLoc)
                    ->libObjPtr());
        },
        "field-class-option-with-selector-field-borrow-selector-field-path-const:mip-version-is-valid");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_borrow_selector_field_location_const(nullptr);
        },
        "field-class-option-with-selector-field-borrow-selector-field-location-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_borrow_selector_field_location_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-with-selector-field-borrow-selector-field-location-const:is-option-field-class-with-selector:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_option_with_selector_field_borrow_selector_field_location_const(
                traceCls
                    ->createOptionWithBoolSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass(),
                        *traceCls->createBoolFieldClass())
                    ->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-borrow-selector-field-location-const:mip-version-is-valid",
        0));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_bool_set_selector_is_reversed(nullptr,
                                                                                    BT_TRUE);
        },
        "field-class-option-with-selector-field-bool-set-selector-is-reversed:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_bool_set_selector_is_reversed(
                createUIntFc(selfComp)->libObjPtr(), BT_TRUE);
        },
        "field-class-option-with-selector-field-bool-set-selector-is-reversed:is-option-field-class-with-boolean-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto optFc = traceCls->createOptionWithBoolSelectorFieldClass(
                *traceCls->createUnsignedIntegerFieldClass(), *traceCls->createBoolFieldClass());

            traceCls->createStructureFieldClass()->appendMember("opt", *optFc);
            bt_field_class_option_with_selector_field_bool_set_selector_is_reversed(
                optFc->libObjPtr(), BT_TRUE);
        },
        CondTrigger::Type::Pre,
        "field-class-option-with-selector-field-bool-set-selector-is-reversed:not-frozen:field-class",
        0));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_bool_selector_is_reversed(nullptr);
        },
        "field-class-option-with-selector-field-bool-selector-is-reversed:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_bool_selector_is_reversed(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-with-selector-field-bool-selector-is-reversed:is-option-field-class-with-boolean-selector-field:field-class",
        0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_integer_unsigned_borrow_selector_ranges_const(
                nullptr);
        },
        "field-class-option-with-selector-field-integer-unsigned-borrow-selector-ranges-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_integer_unsigned_borrow_selector_ranges_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-with-selector-field-integer-unsigned-borrow-selector-ranges-const:is-option-field-class-with-integer-selector:field-class",
        0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_option_with_selector_field_integer_signed_borrow_selector_ranges_const(
                nullptr);
        },
        "field-class-option-with-selector-field-integer-signed-borrow-selector-ranges-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_option_with_selector_field_integer_signed_borrow_selector_ranges_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-option-with-selector-field-integer-signed-borrow-selector-ranges-const:is-option-field-class-with-integer-selector:field-class",
        0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_create(nullptr, nullptr);
        },
        "field-class-variant-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_create(nullptr, nullptr);
        },
        "field-class-variant-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_create(selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        "field-class-variant-create:mip-version-is-valid");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_create(traceCls->libObjPtr(),
                                          traceCls->createBoolFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-create:is-int-field-class:selector-field-class", 0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_without_selector_field_location_create(nullptr);
        },
        "field-class-variant-without-selector-field-location-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_without_selector_field_location_create(nullptr);
        },
        "field-class-variant-without-selector-field-location-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_without_selector_field_location_create(
                selfComp.createTraceClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-without-selector-field-location-create:mip-version-is-valid", 0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_location_integer_unsigned_create(nullptr,
                                                                                        nullptr);
        },
        "field-class-variant-with-selector-field-location-integer-unsigned-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_location_integer_unsigned_create(nullptr,
                                                                                        nullptr);
        },
        "field-class-variant-with-selector-field-location-integer-unsigned-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_location_integer_unsigned_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-location-integer-unsigned-create:mip-version-is-valid",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_location_integer_unsigned_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        "field-class-variant-with-selector-field-location-integer-unsigned-create:not-null:selector-field-location");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_location_integer_signed_create(nullptr,
                                                                                      nullptr);
        },
        "field-class-variant-with-selector-field-location-integer-signed-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_location_integer_signed_create(nullptr,
                                                                                      nullptr);
        },
        "field-class-variant-with-selector-field-location-integer-signed-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_location_integer_signed_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-location-integer-signed-create:mip-version-is-valid",
        0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_location_integer_signed_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        "field-class-variant-with-selector-field-location-integer-signed-create:not-null:selector-field-location");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_without_selector_append_option(nullptr, "x", nullptr);
        },
        "field-class-variant-without-selector-append-option:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_without_selector_append_option(nullptr, "x", nullptr);
        },
        "field-class-variant-without-selector-append-option:not-null:field-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_without_selector_append_option(
                selfComp.createTraceClass()->createVariantFieldClass()->libObjPtr(), nullptr,
                nullptr);
        },
        CondTrigger::Type::Pre, "field-class-variant-without-selector-append-option:not-null:name",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_without_selector_append_option(
                selfComp.createTraceClass()->createVariantFieldClass()->libObjPtr(), "x", nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-without-selector-append-option:not-null:option-field-class", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_without_selector_append_option(
                selfComp.createTraceClass()
                    ->createVariantWithoutSelectorFieldLocationFieldClass()
                    ->libObjPtr(),
                "x", nullptr);
        },
        "field-class-variant-without-selector-append-option:not-null:option-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_without_selector_append_option(
                traceCls->createStructureFieldClass()->libObjPtr(), "x",
                traceCls->createUnsignedIntegerFieldClass()->libObjPtr());
        },
        "field-class-variant-without-selector-append-option:is-variant-field-class-without-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantFieldClass();

            traceCls->createStructureFieldClass()->appendMember("v", *varFc);
            /* `varFc` is now frozen */
            bt_field_class_variant_without_selector_append_option(
                varFc->libObjPtr(), "x", traceCls->createUnsignedIntegerFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-without-selector-append-option:not-frozen:field-class", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantFieldClass();

            varFc->appendOption("dup", *traceCls->createUnsignedIntegerFieldClass());
            varFc->appendOption("dup", *traceCls->createUnsignedIntegerFieldClass());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-without-selector-append-option:variant-field-class-option-name-is-unique",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto memberFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *memberFc);
            traceCls->createVariantFieldClass()->appendOption("opt", *memberFc);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-without-selector-append-option:field-class-is-not-part-of-something",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                nullptr, "x", nullptr, nullptr);
        },
        "field-class-variant-with-selector-field-integer-unsigned-append-option:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                nullptr, "x", nullptr, nullptr);
        },
        "field-class-variant-with-selector-field-integer-unsigned-append-option:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                createUIntFc(selfComp)->libObjPtr(), "x", nullptr, nullptr);
        },
        "field-class-variant-with-selector-field-integer-unsigned-append-option:is-variant-field-class-with-unsigned-integer-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr(),
                nullptr, nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:not-null:name", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr(),
                "x", nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:not-null:option-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr(),
                "x", traceCls->createUnsignedIntegerFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:not-null:integer-range-set",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_unsigned_append_option(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr(),
                "x", traceCls->createUnsignedIntegerFieldClass()->libObjPtr(),
                bt2::UnsignedIntegerRangeSet::create()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:integer-range-set-is-not-empty",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantWithUnsignedIntegerSelectorFieldClass(
                *traceCls->createUnsignedIntegerFieldClass());
            const auto ranges1 = bt2::UnsignedIntegerRangeSet::create();
            const auto ranges2 = bt2::UnsignedIntegerRangeSet::create();

            ranges1->addRange(0, 10);
            ranges2->addRange(5, 15);
            varFc->appendOption("a", *traceCls->createUnsignedIntegerFieldClass(), *ranges1);
            varFc->appendOption("b", *traceCls->createUnsignedIntegerFieldClass(), *ranges2);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:ranges-do-not-overlap",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantWithUnsignedIntegerSelectorFieldClass(
                *traceCls->createUnsignedIntegerFieldClass());
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            traceCls->createStructureFieldClass()->appendMember("v", *varFc);
            varFc->appendOption("x", *traceCls->createUnsignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:not-frozen:field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantWithUnsignedIntegerSelectorFieldClass(
                *traceCls->createUnsignedIntegerFieldClass());
            const auto ranges1 = bt2::UnsignedIntegerRangeSet::create();
            const auto ranges2 = bt2::UnsignedIntegerRangeSet::create();

            ranges1->addRange(0, 0);
            ranges2->addRange(1, 1);
            varFc->appendOption("dup", *traceCls->createUnsignedIntegerFieldClass(), *ranges1);
            varFc->appendOption("dup", *traceCls->createUnsignedIntegerFieldClass(), *ranges2);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:variant-field-class-option-name-is-unique",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto memberFc = traceCls->createUnsignedIntegerFieldClass();
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();
            const auto structFc = traceCls->createStructureFieldClass();

            ranges->addRange(0, 0);
            structFc->appendMember("m", *memberFc);
            traceCls
                ->createVariantWithUnsignedIntegerSelectorFieldClass(
                    *traceCls->createUnsignedIntegerFieldClass())
                ->appendOption("x", *memberFc, *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-append-option:field-class-is-not-part-of-something",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                nullptr, "x", nullptr, nullptr);
        },
        "field-class-variant-with-selector-field-integer-signed-append-option:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                nullptr, "x", nullptr, nullptr);
        },
        "field-class-variant-with-selector-field-integer-signed-append-option:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                createUIntFc(selfComp)->libObjPtr(), "x", nullptr, nullptr);
        },
        "field-class-variant-with-selector-field-integer-signed-append-option:is-variant-field-class-with-signed-integer-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                traceCls
                    ->createVariantWithSignedIntegerSelectorFieldClass(
                        *traceCls->createSignedIntegerFieldClass())
                    ->libObjPtr(),
                nullptr, nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:not-null:name", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                traceCls
                    ->createVariantWithSignedIntegerSelectorFieldClass(
                        *traceCls->createSignedIntegerFieldClass())
                    ->libObjPtr(),
                "x", nullptr, nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:not-null:option-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                traceCls
                    ->createVariantWithSignedIntegerSelectorFieldClass(
                        *traceCls->createSignedIntegerFieldClass())
                    ->libObjPtr(),
                "x", traceCls->createSignedIntegerFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:not-null:integer-range-set",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_signed_append_option(
                traceCls
                    ->createVariantWithSignedIntegerSelectorFieldClass(
                        *traceCls->createSignedIntegerFieldClass())
                    ->libObjPtr(),
                "x", traceCls->createSignedIntegerFieldClass()->libObjPtr(),
                bt2::SignedIntegerRangeSet::create()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:integer-range-set-is-not-empty",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantWithSignedIntegerSelectorFieldClass(
                *traceCls->createSignedIntegerFieldClass());
            const auto ranges1 = bt2::SignedIntegerRangeSet::create();
            const auto ranges2 = bt2::SignedIntegerRangeSet::create();

            ranges1->addRange(0, 10);
            ranges2->addRange(5, 15);
            varFc->appendOption("a", *traceCls->createSignedIntegerFieldClass(), *ranges1);
            varFc->appendOption("b", *traceCls->createSignedIntegerFieldClass(), *ranges2);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:ranges-do-not-overlap",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantWithSignedIntegerSelectorFieldClass(
                *traceCls->createSignedIntegerFieldClass());
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            traceCls->createStructureFieldClass()->appendMember("v", *varFc);
            varFc->appendOption("x", *traceCls->createSignedIntegerFieldClass(), *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:not-frozen:field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantWithSignedIntegerSelectorFieldClass(
                *traceCls->createSignedIntegerFieldClass());
            const auto ranges1 = bt2::SignedIntegerRangeSet::create();
            const auto ranges2 = bt2::SignedIntegerRangeSet::create();

            ranges1->addRange(0, 0);
            ranges2->addRange(1, 1);
            varFc->appendOption("dup", *traceCls->createSignedIntegerFieldClass(), *ranges1);
            varFc->appendOption("dup", *traceCls->createSignedIntegerFieldClass(), *ranges2);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:variant-field-class-option-name-is-unique",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto memberFc = traceCls->createSignedIntegerFieldClass();
            const auto ranges = bt2::SignedIntegerRangeSet::create();
            const auto structFc = traceCls->createStructureFieldClass();

            ranges->addRange(0, 0);
            structFc->appendMember("m", *memberFc);
            traceCls
                ->createVariantWithSignedIntegerSelectorFieldClass(
                    *traceCls->createSignedIntegerFieldClass())
                ->appendOption("x", *memberFc, *ranges);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-append-option:field-class-is-not-part-of-something",
        0));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_get_option_count(nullptr);
        },
        "field-class-variant-get-option-count:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_get_option_count(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-variant-get-option-count:is-variant-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_borrow_option_by_name(nullptr, "x");
        },
        "field-class-variant-borrow-option-by-name:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_borrow_option_by_name_const(nullptr, "x");
        },
        "field-class-variant-borrow-option-by-name-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_name(createUIntFc(selfComp)->libObjPtr(), "x");
        },
        "field-class-variant-borrow-option-by-name:is-variant-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_name_const(createUIntFc(selfComp)->libObjPtr(),
                                                               "x");
        },
        "field-class-variant-borrow-option-by-name-const:is-variant-field-class:field-class", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_name(
                selfComp.createTraceClass()->createVariantFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre, "field-class-variant-borrow-option-by-name:not-null:name", 0,
        "mip0"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_name_const(
                selfComp.createTraceClass()->createVariantFieldClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre, "field-class-variant-borrow-option-by-name-const:not-null:name", 0,
        "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_borrow_option_by_index(nullptr, 0);
        },
        "field-class-variant-borrow-option-by-index:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_borrow_option_by_index_const(nullptr, 0);
        },
        "field-class-variant-borrow-option-by-index-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_index(createUIntFc(selfComp)->libObjPtr(), 0);
        },
        "field-class-variant-borrow-option-by-index:is-variant-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_index_const(createUIntFc(selfComp)->libObjPtr(),
                                                                0);
        },
        "field-class-variant-borrow-option-by-index-const:is-variant-field-class:field-class", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_index(
                selfComp.createTraceClass()->createVariantFieldClass()->libObjPtr(), 0);
        },
        CondTrigger::Type::Pre, "field-class-variant-borrow-option-by-index:valid-index", 0,
        "mip0"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_variant_borrow_option_by_index_const(
                selfComp.createTraceClass()->createVariantFieldClass()->libObjPtr(), 0);
        },
        CondTrigger::Type::Pre, "field-class-variant-borrow-option-by-index-const:valid-index", 0,
        "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_unsigned_borrow_option_by_name_const(
                nullptr, "x");
        },
        "field-class-variant-with-selector-field-integer-unsigned-borrow-option-by-name-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_integer_unsigned_borrow_option_by_name_const(
                createUIntFc(selfComp)->libObjPtr(), "x");
        },
        "field-class-variant-with-selector-field-integer-unsigned-borrow-option-by-name-const:is-variant-field-class-with-unsigned-integer-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_unsigned_borrow_option_by_name_const(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr(),
                nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-borrow-option-by-name-const:not-null:name",
        0, "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_unsigned_borrow_option_by_index_const(
                nullptr, 0);
        },
        "field-class-variant-with-selector-field-integer-unsigned-borrow-option-by-index-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_integer_unsigned_borrow_option_by_index_const(
                createUIntFc(selfComp)->libObjPtr(), 0);
        },
        "field-class-variant-with-selector-field-integer-unsigned-borrow-option-by-index-const:is-variant-field-class-with-unsigned-integer-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_unsigned_borrow_option_by_index_const(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr(),
                0);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-unsigned-borrow-option-by-index-const:valid-index",
        0, "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_signed_borrow_option_by_name_const(
                nullptr, "x");
        },
        "field-class-variant-with-selector-field-integer-signed-borrow-option-by-name-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_integer_signed_borrow_option_by_name_const(
                createUIntFc(selfComp)->libObjPtr(), "x");
        },
        "field-class-variant-with-selector-field-integer-signed-borrow-option-by-name-const:is-variant-field-class-with-signed-integer-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_signed_borrow_option_by_name_const(
                traceCls
                    ->createVariantWithSignedIntegerSelectorFieldClass(
                        *traceCls->createSignedIntegerFieldClass())
                    ->libObjPtr(),
                nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-borrow-option-by-name-const:not-null:name",
        0, "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_signed_borrow_option_by_index_const(
                nullptr, 0);
        },
        "field-class-variant-with-selector-field-integer-signed-borrow-option-by-index-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_integer_signed_borrow_option_by_index_const(
                createUIntFc(selfComp)->libObjPtr(), 0);
        },
        "field-class-variant-with-selector-field-integer-signed-borrow-option-by-index-const:is-variant-field-class-with-signed-integer-selector-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_integer_signed_borrow_option_by_index_const(
                traceCls
                    ->createVariantWithSignedIntegerSelectorFieldClass(
                        *traceCls->createSignedIntegerFieldClass())
                    ->libObjPtr(),
                0);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-integer-signed-borrow-option-by-index-const:valid-index",
        0, "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_option_get_name(nullptr);
        },
        "field-class-variant-option-get-name:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_option_borrow_field_class(nullptr);
        },
        "field-class-variant-option-borrow-field-class:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_option_borrow_field_class_const(nullptr);
        },
        "field-class-variant-option-borrow-field-class-const:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_unsigned_option_borrow_ranges_const(
                nullptr);
        },
        "field-class-variant-with-selector-field-integer-unsigned-option-borrow-ranges-const:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_integer_signed_option_borrow_ranges_const(
                nullptr);
        },
        "field-class-variant-with-selector-field-integer-signed-option-borrow-ranges-const:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_borrow_selector_field_path_const(nullptr);
        },
        "field-class-variant-with-selector-field-borrow-selector-field-path-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_borrow_selector_field_path_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-variant-with-selector-field-borrow-selector-field-path-const:is-variant-field-class-with-selector:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"sel"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_variant_with_selector_field_borrow_selector_field_path_const(
                traceCls->createVariantWithUnsignedIntegerSelectorFieldLocationFieldClass(*fieldLoc)
                    ->libObjPtr());
        },
        "field-class-variant-with-selector-field-borrow-selector-field-path-const:mip-version-is-valid");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_with_selector_field_borrow_selector_field_location_const(
                nullptr);
        },
        "field-class-variant-with-selector-field-borrow-selector-field-location-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_variant_with_selector_field_borrow_selector_field_location_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-variant-with-selector-field-borrow-selector-field-location-const:is-variant-field-class-with-selector:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_variant_with_selector_field_borrow_selector_field_location_const(
                traceCls
                    ->createVariantWithUnsignedIntegerSelectorFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-variant-with-selector-field-borrow-selector-field-location-const:mip-version-is-valid",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_array_static_create(nullptr, nullptr, 0);
        },
        "field-class-array-static-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_static_create(nullptr, nullptr, 0);
        },
        "field-class-array-static-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_static_create(selfComp.createTraceClass()->libObjPtr(), nullptr,
                                               0);
        },
        "field-class-array-static-create:not-null:element-field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto elemFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *elemFc);
            traceCls->createStaticArrayFieldClass(*elemFc, 4);
        },
        "field-class-array-static-create:field-class-is-not-part-of-something", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_borrow_element_field_class(nullptr);
        },
        "field-class-array-borrow-element-field-class:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_borrow_element_field_class_const(nullptr);
        },
        "field-class-array-borrow-element-field-class-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_borrow_element_field_class(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-array-borrow-element-field-class:is-array-field-class:field-class", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_borrow_element_field_class_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-array-borrow-element-field-class-const:is-array-field-class:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_static_get_length(nullptr);
        },
        "field-class-array-static-get-length:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_static_get_length(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-array-static-get-length:is-static-array-field-class:field-class", 0);

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_create(nullptr, nullptr, nullptr);
        },
        "field-class-array-dynamic-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_create(nullptr, nullptr, nullptr);
        },
        "field-class-array-dynamic-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_dynamic_create(selfComp.createTraceClass()->libObjPtr(), nullptr,
                                                nullptr);
        },
        "field-class-array-dynamic-create:mip-version-is-valid");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_array_dynamic_create(selfComp.createTraceClass()->libObjPtr(), nullptr,
                                                nullptr);
        },
        CondTrigger::Type::Pre, "field-class-array-dynamic-create:not-null:element-field-class",
        0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto elemFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *elemFc);
            traceCls->createDynamicArrayFieldClass(*elemFc);
        },
        CondTrigger::Type::Pre,
        "field-class-array-dynamic-create:field-class-is-not-part-of-something", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_array_dynamic_create(
                traceCls->libObjPtr(), traceCls->createUnsignedIntegerFieldClass()->libObjPtr(),
                traceCls->createSignedIntegerFieldClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-array-dynamic-create:is-unsigned-integer-field-class:length-field-class", 0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_without_length_field_location_create(nullptr, nullptr);
        },
        "field-class-array-dynamic-without-length-field-location-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_without_length_field_location_create(nullptr, nullptr);
        },
        "field-class-array-dynamic-without-length-field-location-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_array_dynamic_without_length_field_location_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-array-dynamic-without-length-field-location-create:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_dynamic_without_length_field_location_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        "field-class-array-dynamic-without-length-field-location-create:not-null:element-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto elemFc = traceCls->createUnsignedIntegerFieldClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *elemFc);
            traceCls->createDynamicArrayWithoutLengthFieldLocationFieldClass(*elemFc);
        },
        "field-class-array-dynamic-without-length-field-location-create:field-class-is-not-part-of-something");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_with_length_field_location_create(nullptr, nullptr,
                                                                           nullptr);
        },
        "field-class-array-dynamic-with-length-field-location-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_with_length_field_location_create(nullptr, nullptr,
                                                                           nullptr);
        },
        "field-class-array-dynamic-with-length-field-location-create:not-null:trace-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_dynamic_with_length_field_location_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr, nullptr);
        },
        "field-class-array-dynamic-with-length-field-location-create:not-null:field-location", 0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            /*
             * bt_field_location_create() requires MIP 1+, but we're in
             * MIP 0, therefore creating one would itself fail.
             *
             * We instead forge an arbitrary non- `NULL` field location
             * pointer below, passing one through the API.
             *
             * To avoid invalid pointer issues, we use a value that's
             * only non- `NULL` for the duration of the assertion.
             * However, BT_ASSERT_PRE_FL_NON_NULL() only checks for
             * `NULL`, so any non- `NULL` pointer suffices for the
             * precondition under test (the program aborts at
             * BT_ASSERT_PRE_TC_MIP_VERSION_GE() before the location
             * is dereferenced).
             */
            bt_field_class_array_dynamic_with_length_field_location_create(
                traceCls->libObjPtr(), nullptr,
                reinterpret_cast<bt_field_location *>(std::uintptr_t {1}));
        },
        CondTrigger::Type::Pre,
        "field-class-array-dynamic-with-length-field-location-create:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"len"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_array_dynamic_with_length_field_location_create(
                traceCls->libObjPtr(), nullptr, fieldLoc->libObjPtr());
        },
        "field-class-array-dynamic-with-length-field-location-create:not-null:element-field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto elemFc = traceCls->createUnsignedIntegerFieldClass();
            const char * const items[] = {"len"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *elemFc);
            traceCls->createDynamicArrayWithLengthFieldLocationFieldClass(*elemFc, *fieldLoc);
        },
        "field-class-array-dynamic-with-length-field-location-create:field-class-is-not-part-of-something");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_with_length_field_borrow_length_field_path_const(nullptr);
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-path-const:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_with_length_field_borrow_length_field_path_const(nullptr);
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-path-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_dynamic_with_length_field_borrow_length_field_path_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-path-const:is-dynamic-array-field-class-with-length-field:field-class",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const char * const items[] = {"len"};
            const auto fieldLoc =
                traceCls->createFieldLocation(bt2::ConstFieldLocation::Scope::PacketContext,
                                              bt2s::span<const char * const> {items, 1});

            bt_field_class_array_dynamic_with_length_field_borrow_length_field_path_const(
                traceCls
                    ->createDynamicArrayWithLengthFieldLocationFieldClass(
                        *traceCls->createUnsignedIntegerFieldClass(), *fieldLoc)
                    ->libObjPtr());
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-path-const:mip-version-is-valid");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_with_length_field_borrow_length_field_location_const(
                nullptr);
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-location-const:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_array_dynamic_with_length_field_borrow_length_field_location_const(
                nullptr);
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-location-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_array_dynamic_with_length_field_borrow_length_field_location_const(
                createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-array-dynamic-with-length-field-borrow-length-field-location-const:is-dynamic-array-field-class-with-length-field:field-class",
        0);

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_array_dynamic_with_length_field_borrow_length_field_location_const(
                traceCls
                    ->createDynamicArrayFieldClass(*traceCls->createUnsignedIntegerFieldClass(),
                                                   *traceCls->createUnsignedIntegerFieldClass())
                    ->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-array-dynamic-with-length-field-borrow-length-field-location-const:mip-version-is-valid",
        0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_string_create(nullptr);
        },
        "field-class-string-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_string_create(nullptr);
        },
        "field-class-string-create:not-null:trace-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_borrow_user_attributes(nullptr);
        },
        "field-class-borrow-user-attributes:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_borrow_user_attributes_const(nullptr);
        },
        "field-class-borrow-user-attributes-const:not-null:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_set_user_attributes(nullptr, nullptr);
        },
        "field-class-set-user-attributes:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_set_user_attributes(createUIntFc(selfComp)->libObjPtr(), nullptr);
        },
        "field-class-set-user-attributes:not-null:user-attributes-value-object", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto val = bt_value_bool_create();

            bt_field_class_set_user_attributes(createUIntFc(selfComp)->libObjPtr(), val);
            bt_value_put_ref(val);
        },
        "field-class-set-user-attributes:is-map-value:user-attributes", 0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto val = bt_value_map_create();

            bt_field_class_set_user_attributes(createFrozenUIntFc(selfComp)->libObjPtr(), val);
            bt_value_put_ref(val);
        },
        "field-class-set-user-attributes:not-frozen:field-class", 0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_member_borrow_user_attributes(nullptr);
        },
        "field-class-structure-member-borrow-user-attributes:not-null:structure-field-class-member");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_member_borrow_user_attributes_const(nullptr);
        },
        "field-class-structure-member-borrow-user-attributes-const:not-null:structure-field-class-member");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_structure_member_set_user_attributes(nullptr, nullptr);
        },
        "field-class-structure-member-set-user-attributes:not-null:structure-field-class-member");

    /*
     * `not-frozen:structure-field-class-member`: freeze the parent
     * structure field class so the member's `frozen` fieldLocag becomes set.
     */
    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto structFc = traceCls->createStructureFieldClass();
            const auto val = bt_value_map_create();

            structFc->appendMember("m", *traceCls->createUnsignedIntegerFieldClass());
            traceCls->createStructureFieldClass()->appendMember("s", *structFc);
            /* `structFc`'s member is now frozen. */
            bt_field_class_structure_member_set_user_attributes(
                bt_field_class_structure_borrow_member_by_index(structFc->libObjPtr(), 0), val);
            bt_value_put_ref(val);
        },
        "field-class-structure-member-set-user-attributes:not-frozen:structure-field-class-member",
        0);

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto structFc = traceCls->createStructureFieldClass();

            structFc->appendMember("m", *traceCls->createUnsignedIntegerFieldClass());
            bt_field_class_structure_member_set_user_attributes(
                bt_field_class_structure_borrow_member_by_index(structFc->libObjPtr(), 0), nullptr);
        },
        "field-class-structure-member-set-user-attributes:not-null:user-attributes-value-object",
        0);

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_option_borrow_user_attributes(nullptr);
        },
        "field-class-variant-option-borrow-user-attributes:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_option_borrow_user_attributes_const(nullptr);
        },
        "field-class-variant-option-borrow-user-attributes-const:not-null:variant-field-class-option-id");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_variant_option_set_user_attributes(nullptr, nullptr);
        },
        "field-class-variant-option-set-user-attributes:not-null:variant-field-class-option-id");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantFieldClass();
            const auto val = bt_value_map_create();

            varFc->appendOption("o", *traceCls->createUnsignedIntegerFieldClass());
            traceCls->createStructureFieldClass()->appendMember("v", *varFc);
            /* The variant FC option is now frozen. */
            bt_field_class_variant_option_set_user_attributes(
                bt_field_class_variant_borrow_option_by_index(varFc->libObjPtr(), 0), val);
            bt_value_put_ref(val);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-option-set-user-attributes:not-frozen:variant-field-class-option", 0,
        "mip0"));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto varFc = traceCls->createVariantFieldClass();

            varFc->appendOption("o", *traceCls->createUnsignedIntegerFieldClass());
            bt_field_class_variant_option_set_user_attributes(
                bt_field_class_variant_borrow_option_by_index(varFc->libObjPtr(), 0), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-variant-option-set-user-attributes:not-null:user-attributes-value-object", 0,
        "mip0"));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_set_media_type(nullptr, "x");
        },
        "field-class-blob-set-media-type:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_blob_set_media_type(createUIntFc(selfComp)->libObjPtr(), "x");
        },
        "field-class-blob-set-media-type:is-blob-field-class:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();
            const auto blobFc = traceCls->createStaticBlobFieldClass(8);

            traceCls->createStructureFieldClass()->appendMember("b", *blobFc);
            bt_field_class_blob_set_media_type(blobFc->libObjPtr(), "x");
        },
        "field-class-blob-set-media-type:not-frozen:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_blob_set_media_type(
                selfComp.createTraceClass()->createStaticBlobFieldClass(8)->libObjPtr(), nullptr);
        },
        "field-class-blob-set-media-type:not-null:media-type");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_get_media_type(nullptr);
        },
        "field-class-blob-get-media-type:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_blob_get_media_type(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-blob-get-media-type:is-blob-field-class:field-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_blob_static_create(nullptr, 0);
        },
        "field-class-blob-static-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_static_create(nullptr, 0);
        },
        "field-class-blob-static-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_blob_static_create(selfComp.createTraceClass()->libObjPtr(), 0);
        },
        CondTrigger::Type::Pre, "field-class-blob-static-create:mip-version-is-valid", 0));

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_static_get_length(nullptr);
        },
        "field-class-blob-static-get-length:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_blob_static_get_length(createUIntFc(selfComp)->libObjPtr());
        },
        "field-class-blob-static-get-length:is-static-blob-field-class:field-class");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_blob_dynamic_without_length_field_location_create(nullptr);
        },
        "field-class-blob-dynamic-without-length-field-location-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_dynamic_without_length_field_location_create(nullptr);
        },
        "field-class-blob-dynamic-without-length-field-location-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_blob_dynamic_without_length_field_location_create(
                selfComp.createTraceClass()->libObjPtr());
        },
        CondTrigger::Type::Pre,
        "field-class-blob-dynamic-without-length-field-location-create:mip-version-is-valid", 0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_blob_dynamic_with_length_field_location_create(nullptr, nullptr);
        },
        "field-class-blob-dynamic-with-length-field-location-create:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_dynamic_with_length_field_location_create(nullptr, nullptr);
        },
        "field-class-blob-dynamic-with-length-field-location-create:not-null:trace-class");

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            bt_field_class_blob_dynamic_with_length_field_location_create(
                selfComp.createTraceClass()->libObjPtr(), nullptr);
        },
        CondTrigger::Type::Pre,
        "field-class-blob-dynamic-with-length-field-location-create:not-null:field-location", 0));

    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const auto traceCls = selfComp.createTraceClass();

            bt_field_class_blob_dynamic_with_length_field_location_create(
                traceCls->libObjPtr(), reinterpret_cast<bt_field_location *>(std::uintptr_t {1}));
        },
        CondTrigger::Type::Pre,
        "field-class-blob-dynamic-with-length-field-location-create:mip-version-is-valid", 0));

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_class_blob_dynamic_with_length_field_borrow_length_field_location_const(
                nullptr);
        },
        "field-class-blob-dynamic-with-length-field-borrow-length-field-location-const:no-error");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_blob_dynamic_with_length_field_borrow_length_field_location_const(
                nullptr);
        },
        "field-class-blob-dynamic-with-length-field-borrow-length-field-location-const:not-null:field-class");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_class_blob_dynamic_with_length_field_borrow_length_field_location_const(
                selfComp.createTraceClass()->createStaticBlobFieldClass(8)->libObjPtr());
        },
        "field-class-blob-dynamic-with-length-field-borrow-length-field-location-const:is-dynamic-blob-field-class-with-length-field:field-class");

    addPreTrigger(
        triggers,
        [] {
            bt_field_class_get_graph_mip_version(nullptr);
        },
        "field-class-get-graph-mip-version:not-null:field-class");
}
