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

#include "cpp-common/bt2/field-class.hpp"
#include "cpp-common/bt2/integer-range-set.hpp"
#include "cpp-common/bt2/trace-ir.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

namespace {

using BuildFcFunc = std::function<bt2::StructureFieldClass::Shared(bt2::TraceClass)>;
using FieldTripFunc = std::function<void(bt_field *)>;

/*
 * Adds an event payload field trigger.
 *
 * Builds a trace IR with a single event class whose payload field class
 * is produced by `buildPayloadFcFunc`, instantiates an event message
 * from that class, then calls `tripFunc` with the raw `bt_field`
 * pointer of the resulting payload field.
 */
void addEventPayloadFieldTrigger(CondTriggers& triggers, BuildFcFunc buildPayloadFcFunc,
                                 FieldTripFunc tripFunc, const std::string& condId,
                                 const std::uint64_t mipVersion = 0,
                                 const std::string_view nameSuffix = {})
{
    triggers.emplace_back(makeRunInMsgIterInitTrigger(
        [buildPayloadFcFunc = std::move(buildPayloadFcFunc),
         tripFunc = std::move(tripFunc)](const auto selfMsgIter) {
            const auto traceCls = selfMsgIter.component().createTraceClass();
            const auto streamCls = traceCls->createStreamClass();
            const auto eventCls = streamCls->createEventClass();

            eventCls->payloadFieldClass(*buildPayloadFcFunc(*traceCls));

            const auto field =
                (*selfMsgIter
                      .createEventMessage(*eventCls,
                                          *streamCls->instantiate(*traceCls->instantiate()))
                      ->event()
                      .payloadField())
                    .libObjPtr();

            tripFunc(field);
        },
        CondTrigger::Type::Pre, condId, mipVersion, nameSuffix));
}

/*
 * Adds a frozen packet context field trigger.
 *
 * Builds a trace IR (MIP 0) where the stream class supports packets and
 * its packet context field class is produced by `buildPacketCtxFc`,
 * creates a packet beginning message (which freezes the packet and its
 * context field), then calls `tripFunc` with the raw `bt_field` pointer
 * of the now-frozen packet context field.
 */
void addFrozenPacketCtxFieldTrigger(CondTriggers& triggers, BuildFcFunc buildPacketCtxFc,
                                    FieldTripFunc tripFunc, const std::string& condId,
                                    const std::uint64_t mipVersion = 0,
                                    const std::string_view nameSuffix = {})
{
    triggers.emplace_back(makeRunInMsgIterInitTrigger(
        [buildPacketCtxFc = std::move(buildPacketCtxFc),
         tripFunc = std::move(tripFunc)](const auto selfMsgIter) {
            const auto traceCls = selfMsgIter.component().createTraceClass();
            const auto streamCls = traceCls->createStreamClass();

            streamCls->supportsPackets(true, false, false);
            streamCls->packetContextFieldClass(*buildPacketCtxFc(*traceCls));

            const auto trace = traceCls->instantiate();
            const auto stream = streamCls->instantiate(*trace);
            const auto packet = stream->createPacket();

            /*
             * Borrow the packet context field _before_ creating the
             * packet beginning message: the message creation will
             * freeze the packet (and recurse through every member of
             * its context field).
             */
            const auto ctxField = (*packet->contextField()).libObjPtr();

            /* Freeze the packet (and its context field) */
            static_cast<void>(selfMsgIter.createPacketBeginningMessage(*packet));

            /* Run the test against the now-frozen context field */
            tripFunc(ctxField);
        },
        CondTrigger::Type::Pre, condId, mipVersion, nameSuffix));
}

/*
 * Builds a structure field class with one member named `f` of
 * class `memberFc`.
 */
bt2::StructureFieldClass::Shared singleMemberStructFc(const bt2::TraceClass traceCls,
                                                      const bt2::FieldClass memberFc)
{
    const auto fc = traceCls.createStructureFieldClass();

    fc->appendMember("f", memberFc);
    return fc;
}

/*
 * Returns the libbabeltrace2 pointer of the `f` member field of a
 * structure field.
 */
bt_field *firstMember(bt_field * const structField) noexcept
{
    return bt_field_structure_borrow_member_field_by_index(structField, 0);
}

/*
 * Adds an `is-*-field:field` trigger.
 *
 * Builds a structure field with a single member of the wrong field
 * class produced by `wrongMemberFcFactoryFunc`, calls
 * `preSetMemberFunc` on that member (to bypass the `is-field-set:field`
 * precondition when the target API checks it before the type), and then
 * calls `tripFunc` on the same member to trip the target precondition.
 */
template <typename WrongMemberFcFactoryFunc, typename PreSetFunc, typename TripFunc>
void addWrongFieldTypeTrigger(CondTriggers& triggers,
                              WrongMemberFcFactoryFunc wrongMemberFcFactoryFunc,
                              PreSetFunc preSetMemberFunc, TripFunc tripFunc,
                              const std::string& condId, const std::uint64_t mipVersion = 0)
{
    addEventPayloadFieldTrigger(
        triggers,
        [factoryFunc = std::move(wrongMemberFcFactoryFunc)](const auto traceCls) {
            return singleMemberStructFc(traceCls, *factoryFunc(traceCls));
        },
        [preSetFunc = std::move(preSetMemberFunc),
         call = std::move(tripFunc)](const auto libStructFieldPtr) {
            const auto field = firstMember(libStructFieldPtr);

            preSetFunc(field);
            call(field);
        },
        condId, mipVersion);
}

/*
 * Adds a `not-frozen:field` trigger.
 *
 * Builds a packet context whose single member field class is produced
 * by `ctxMemberFcFactoryFunc` (so creating the packet beginning message
 * freezes that member), then calls `tripFunc` on the (now frozen) member
 * to trip the target precondition.
 */
template <typename CtxMemberFcFactoryFunc, typename TripFunc>
void addNotFrozenFieldTrigger(CondTriggers& triggers, CtxMemberFcFactoryFunc ctxMemberFcFactoryFunc,
                              TripFunc tripFunc, const std::string& condId,
                              const std::uint64_t mipVersion = 0)
{
    addFrozenPacketCtxFieldTrigger(
        triggers,
        [factoryFunc = std::move(ctxMemberFcFactoryFunc)](const auto traceCls) {
            return singleMemberStructFc(traceCls, *factoryFunc(traceCls));
        },
        [tripFunc = std::move(tripFunc)](const auto libCtxFieldPtr) {
            tripFunc(firstMember(libCtxFieldPtr));
        },
        condId, mipVersion);
}

} /* namespace */

/*
 * Adds field API precondition failure triggers.
 */
void addFieldPreCondsTriggers(CondTriggers& triggers)
{
    addPreTrigger(
        triggers,
        [] {
            bt_field_borrow_class(nullptr);
        },
        "field-borrow-class:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_borrow_class_const(nullptr);
        },
        "field-borrow-class-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_get_class_type(nullptr);
        },
        "field-get-class-type:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_bool_get_value(nullptr);
        },
        "field-bool-get-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_bool_set_value(nullptr, BT_TRUE);
        },
        "field-bool-set-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_bit_array_get_value_as_integer(nullptr);
        },
        "field-bit-array-get-value-as-integer:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_bit_array_set_value_as_integer(nullptr, 0);
        },
        "field-bit-array-set-value-as-integer:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_bit_array_get_active_flag_labels(nullptr, nullptr, nullptr);
        },
        "field-bit-array-get-active-flag-labels:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_integer_signed_get_value(nullptr);
        },
        "field-integer-signed-get-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_integer_signed_set_value(nullptr, 0);
        },
        "field-integer-signed-set-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_integer_unsigned_get_value(nullptr);
        },
        "field-integer-unsigned-get-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_integer_unsigned_set_value(nullptr, 0);
        },
        "field-integer-unsigned-set-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_real_single_precision_get_value(nullptr);
        },
        "field-real-single-precision-get-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_real_double_precision_get_value(nullptr);
        },
        "field-real-double-precision-get-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_real_single_precision_set_value(nullptr, 0.0f);
        },
        "field-real-single-precision-set-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_real_double_precision_set_value(nullptr, 0.0);
        },
        "field-real-double-precision-set-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_enumeration_unsigned_get_mapping_labels(nullptr, nullptr, nullptr);
        },
        "field-enumeration-unsigned-get-mapping-labels:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_enumeration_signed_get_mapping_labels(nullptr, nullptr, nullptr);
        },
        "field-enumeration-signed-get-mapping-labels:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_get_value(nullptr);
        },
        "field-string-get-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_get_length(nullptr);
        },
        "field-string-get-length:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_set_value(nullptr, "x");
        },
        "field-string-set-value:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_append_with_length(nullptr, "x", 1);
        },
        "field-string-append-with-length:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_append(nullptr, "x");
        },
        "field-string-append:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_clear(nullptr);
        },
        "field-string-clear:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_array_get_length(nullptr);
        },
        "field-array-get-length:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_array_dynamic_set_length(nullptr, 0);
        },
        "field-array-dynamic-set-length:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_array_borrow_element_field_by_index(nullptr, 0);
        },
        "field-array-borrow-element-field-by-index:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_array_borrow_element_field_by_index_const(nullptr, 0);
        },
        "field-array-borrow-element-field-by-index-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_structure_borrow_member_field_by_index(nullptr, 0);
        },
        "field-structure-borrow-member-field-by-index:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_structure_borrow_member_field_by_index_const(nullptr, 0);
        },
        "field-structure-borrow-member-field-by-index-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_structure_borrow_member_field_by_name(nullptr, "f");
        },
        "field-structure-borrow-member-field-by-name:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_structure_borrow_member_field_by_name_const(nullptr, "f");
        },
        "field-structure-borrow-member-field-by-name-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_option_set_has_field(nullptr, BT_FALSE);
        },
        "field-option-set-has-field:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_option_borrow_field(nullptr);
        },
        "field-option-borrow-field:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_borrow_selected_option_field(nullptr);
        },
        "field-variant-borrow-selected-option-field:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_borrow_selected_option_field_const(nullptr);
        },
        "field-variant-borrow-selected-option-field-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_borrow_selected_option_class_const(nullptr);
        },
        "field-variant-borrow-selected-option-class-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_with_selector_field_integer_unsigned_borrow_selected_option_class_const(
                nullptr);
        },
        "field-variant-with-selector-field-integer-unsigned-borrow-selected-option-class-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_with_selector_field_integer_signed_borrow_selected_option_class_const(
                nullptr);
        },
        "field-variant-with-selector-field-integer-signed-borrow-selected-option-class-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_select_option_by_index(nullptr, 0);
        },
        "field-variant-select-option-by-index:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_variant_get_selected_option_index(nullptr);
        },
        "field-variant-get-selected-option-index:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_blob_get_data(nullptr);
        },
        "field-blob-get-data:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_blob_get_data_const(nullptr);
        },
        "field-blob-get-data-const:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_blob_dynamic_set_length(nullptr, 0);
        },
        "field-blob-dynamic-set-length:not-null:field");

    addPreTrigger(
        triggers,
        [] {
            bt_field_blob_get_length(nullptr);
        },
        "field-blob-get-length:not-null:field");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_bit_array_get_active_flag_labels(nullptr, nullptr, nullptr);
        },
        "field-bit-array-get-active-flag-labels:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_enumeration_unsigned_get_mapping_labels(nullptr, nullptr, nullptr);
        },
        "field-enumeration-unsigned-get-mapping-labels:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_enumeration_signed_get_mapping_labels(nullptr, nullptr, nullptr);
        },
        "field-enumeration-signed-get-mapping-labels:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_string_set_value(nullptr, nullptr);
        },
        "field-string-set-value:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_string_append_with_length(nullptr, nullptr, 0);
        },
        "field-string-append-with-length:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_array_dynamic_set_length(nullptr, 0);
        },
        "field-array-dynamic-set-length:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_variant_select_option_by_index(nullptr, 0);
        },
        "field-variant-select-option-by-index:no-error");

    addPreNoErrorTrigger(
        triggers,
        [] {
            bt_field_blob_dynamic_set_length(nullptr, 0);
        },
        "field-blob-dynamic-set-length:no-error");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStringFieldClass());
        },
        [](const auto libStructFieldPtr) {
            withCurrentThreadError([&] {
                bt_field_string_append(firstMember(libStructFieldPtr), "x");
            });
        },
        "field-string-append:no-error");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createBitArrayFieldClass(8));
        },
        [](const auto libStructFieldPtr) {
            const auto bitArrayField = firstMember(libStructFieldPtr);

            bt_field_bit_array_set_value_as_integer(bitArrayField, 0);

            std::uint64_t count = 0;

            bt_field_bit_array_get_active_flag_labels(bitArrayField, nullptr, &count);
        },
        "field-bit-array-get-active-flag-labels:not-null:label-array-output", 1);

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createBitArrayFieldClass(8));
        },
        [](const auto libStructFieldPtr) {
            const auto bitArrayField = firstMember(libStructFieldPtr);

            bt_field_bit_array_set_value_as_integer(bitArrayField, 0);

            bt_field_class_bit_array_flag_label_array labels;

            bt_field_bit_array_get_active_flag_labels(bitArrayField, &labels, nullptr);
        },
        "field-bit-array-get-active-flag-labels:not-null:count-output", 1);

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedEnumerationFieldClass());
        },
        [](const auto libStructFieldPtr) {
            std::uint64_t count = 0;

            bt_field_enumeration_unsigned_get_mapping_labels(firstMember(libStructFieldPtr),
                                                             nullptr, &count);
        },
        "field-enumeration-unsigned-get-mapping-labels:not-null:label-array-output");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedEnumerationFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_class_enumeration_mapping_label_array labels;

            bt_field_enumeration_unsigned_get_mapping_labels(firstMember(libStructFieldPtr),
                                                             &labels, nullptr);
        },
        "field-enumeration-unsigned-get-mapping-labels:not-null:count-output");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createSignedEnumerationFieldClass());
        },
        [](const auto libStructFieldPtr) {
            std::uint64_t count = 0;

            bt_field_enumeration_signed_get_mapping_labels(firstMember(libStructFieldPtr), nullptr,
                                                           &count);
        },
        "field-enumeration-signed-get-mapping-labels:not-null:label-array-output");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createSignedEnumerationFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_class_enumeration_mapping_label_array labels;

            bt_field_enumeration_signed_get_mapping_labels(firstMember(libStructFieldPtr), &labels,
                                                           nullptr);
        },
        "field-enumeration-signed-get-mapping-labels:not-null:count-output");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStringFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_string_set_value(firstMember(libStructFieldPtr), nullptr);
        },
        "field-string-set-value:not-null:value");

    addPreTrigger(
        triggers,
        [] {
            bt_field_string_append(reinterpret_cast<bt_field *>(0xDEADBEEFul), nullptr);
        },
        "field-string-append:not-null:value");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_structure_borrow_member_field_by_name(libStructFieldPtr, nullptr);
        },
        "field-structure-borrow-member-field-by-name:not-null:member-name");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_structure_borrow_member_field_by_name_const(libStructFieldPtr, nullptr);
        },
        "field-structure-borrow-member-field-by-name-const:not-null:member-name");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createBoolFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_bool_get_value(firstMember(libStructFieldPtr));
        },
        "field-bool-get-value:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createBitArrayFieldClass(8));
        },
        [](const auto libStructFieldPtr) {
            bt_field_bit_array_get_value_as_integer(firstMember(libStructFieldPtr));
        },
        "field-bit-array-get-value-as-integer:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createBitArrayFieldClass(8));
        },
        [](const auto libStructFieldPtr) {
            bt_field_class_bit_array_flag_label_array labels;
            std::uint64_t count = 0;

            bt_field_bit_array_get_active_flag_labels(firstMember(libStructFieldPtr), &labels,
                                                      &count);
        },
        "field-bit-array-get-active-flag-labels:is-field-set:field", 1);

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createSignedIntegerFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_integer_signed_get_value(firstMember(libStructFieldPtr));
        },
        "field-integer-signed-get-value:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_integer_unsigned_get_value(firstMember(libStructFieldPtr));
        },
        "field-integer-unsigned-get-value:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createSinglePrecisionRealFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_real_single_precision_get_value(firstMember(libStructFieldPtr));
        },
        "field-real-single-precision-get-value:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createDoublePrecisionRealFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_real_double_precision_get_value(firstMember(libStructFieldPtr));
        },
        "field-real-double-precision-get-value:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedEnumerationFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_class_enumeration_mapping_label_array labels;
            std::uint64_t count = 0;

            bt_field_enumeration_unsigned_get_mapping_labels(firstMember(libStructFieldPtr),
                                                             &labels, &count);
        },
        "field-enumeration-unsigned-get-mapping-labels:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createSignedEnumerationFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_class_enumeration_mapping_label_array labels;
            std::uint64_t count = 0;

            bt_field_enumeration_signed_get_mapping_labels(firstMember(libStructFieldPtr), &labels,
                                                           &count);
        },
        "field-enumeration-signed-get-mapping-labels:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStringFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_string_get_value(firstMember(libStructFieldPtr));
        },
        "field-string-get-value:is-field-set:field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStringFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_string_get_length(firstMember(libStructFieldPtr));
        },
        "field-string-get-length:is-field-set:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createUnsignedIntegerFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_integer_unsigned_set_value(libFieldPtr, 0);
        },
        [](const auto libFieldPtr) {
            bt_field_bool_get_value(libFieldPtr);
        },
        "field-bool-get-value:is-boolean-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createUnsignedIntegerFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_TRUE);
        },
        "field-bool-set-value:is-boolean-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createUnsignedIntegerFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_integer_unsigned_set_value(libFieldPtr, 0);
        },
        [](const auto libFieldPtr) {
            bt_field_bit_array_get_value_as_integer(libFieldPtr);
        },
        "field-bit-array-get-value-as-integer:is-bit-array-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createUnsignedIntegerFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_bit_array_set_value_as_integer(libFieldPtr, 0);
        },
        "field-bit-array-set-value-as-integer:is-bit-array-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createUnsignedIntegerFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_integer_unsigned_set_value(libFieldPtr, 0);
        },
        [](const auto libFieldPtr) {
            bt_field_class_bit_array_flag_label_array labels;
            std::uint64_t count = 0;

            bt_field_bit_array_get_active_flag_labels(libFieldPtr, &labels, &count);
        },
        "field-bit-array-get-active-flag-labels:is-bit-array-field:field", 1);

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_integer_signed_get_value(libFieldPtr);
        },
        "field-integer-signed-get-value:is-signed-integer-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_integer_signed_set_value(libFieldPtr, 0);
        },
        "field-integer-signed-set-value:is-signed-integer-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_integer_unsigned_get_value(libFieldPtr);
        },
        "field-integer-unsigned-get-value:is-unsigned-integer-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_integer_unsigned_set_value(libFieldPtr, 0);
        },
        "field-integer-unsigned-set-value:is-unsigned-integer-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_real_single_precision_get_value(libFieldPtr);
        },
        "field-real-single-precision-get-value:is-single-precision-real-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_real_double_precision_get_value(libFieldPtr);
        },
        "field-real-double-precision-get-value:is-double-precision-real-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_real_single_precision_set_value(libFieldPtr, 0.0f);
        },
        "field-real-single-precision-set-value:is-single-precision-real-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_real_double_precision_set_value(libFieldPtr, 0.0);
        },
        "field-real-double-precision-set-value:is-double-precision-real-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_class_enumeration_mapping_label_array labels;
            std::uint64_t count = 0;

            bt_field_enumeration_unsigned_get_mapping_labels(libFieldPtr, &labels, &count);
        },
        "field-enumeration-unsigned-get-mapping-labels:is-unsigned-enumeration-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_class_enumeration_mapping_label_array labels;
            std::uint64_t count = 0;

            bt_field_enumeration_signed_get_mapping_labels(libFieldPtr, &labels, &count);
        },
        "field-enumeration-signed-get-mapping-labels:is-signed-enumeration-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_string_get_value(libFieldPtr);
        },
        "field-string-get-value:is-string-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_FALSE);
        },
        [](const auto libFieldPtr) {
            bt_field_string_get_length(libFieldPtr);
        },
        "field-string-get-length:is-string-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_string_set_value(libFieldPtr, "x");
        },
        "field-string-set-value:is-string-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_string_append_with_length(libFieldPtr, "x", 1);
        },
        "field-string-append-with-length:is-string-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_string_append(libFieldPtr, "x");
        },
        "field-string-append:is-string-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_string_clear(libFieldPtr);
        },
        "field-string-clear:is-string-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_array_get_length(libFieldPtr);
        },
        "field-array-get-length:is-array-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStaticArrayFieldClass(*traceCls.createUnsignedIntegerFieldClass(),
                                                        1);
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_array_dynamic_set_length(libFieldPtr, 0);
        },
        "field-array-dynamic-set-length:is-dynamic-array-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_array_borrow_element_field_by_index(libFieldPtr, 0);
        },
        "field-array-borrow-element-field-by-index:is-array-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_array_borrow_element_field_by_index_const(libFieldPtr, 0);
        },
        "field-array-borrow-element-field-by-index-const:is-array-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_structure_borrow_member_field_by_index(libFieldPtr, 0);
        },
        "field-structure-borrow-member-field-by-index:is-structure-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_structure_borrow_member_field_by_index_const(libFieldPtr, 0);
        },
        "field-structure-borrow-member-field-by-index-const:is-structure-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_structure_borrow_member_field_by_name(libFieldPtr, "x");
        },
        "field-structure-borrow-member-field-by-name:is-structure-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_structure_borrow_member_field_by_name_const(libFieldPtr, "x");
        },
        "field-structure-borrow-member-field-by-name-const:is-structure-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_option_set_has_field(libFieldPtr, BT_FALSE);
        },
        "field-option-set-has-field:is-option-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_option_borrow_field(libFieldPtr);
        },
        "field-option-borrow-field:is-option-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_borrow_selected_option_field(libFieldPtr);
        },
        "field-variant-borrow-selected-option-field:is-variant-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_borrow_selected_option_field_const(libFieldPtr);
        },
        "field-variant-borrow-selected-option-field-const:is-variant-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_borrow_selected_option_class_const(libFieldPtr);
        },
        "field-variant-borrow-selected-option-class-const:is-variant-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_with_selector_field_integer_unsigned_borrow_selected_option_class_const(
                libFieldPtr);
        },
        "field-variant-with-selector-field-integer-unsigned-borrow-selected-option-class-const:is-variant-field-with-unsigned-selector-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_with_selector_field_integer_signed_borrow_selected_option_class_const(
                libFieldPtr);
        },
        "field-variant-with-selector-field-integer-signed-borrow-selected-option-class-const:is-variant-field-with-signed-selector-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_select_option_by_index(libFieldPtr, 0);
        },
        "field-variant-select-option-by-index:is-variant-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_variant_get_selected_option_index(libFieldPtr);
        },
        "field-variant-get-selected-option-index:is-variant-field:field");

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_blob_get_data(libFieldPtr);
        },
        "field-blob-get-data:is-blob-field:field", 1);

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_blob_get_data_const(libFieldPtr);
        },
        "field-blob-get-data-const:is-blob-field:field", 1);

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStaticBlobFieldClass(4);
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_blob_dynamic_set_length(libFieldPtr, 0);
        },
        "field-blob-dynamic-set-length:is-dynamic-blob-field:field", 1);

    addWrongFieldTypeTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](auto) {
        },
        [](const auto libFieldPtr) {
            bt_field_blob_get_length(libFieldPtr);
        },
        "field-blob-get-length:is-blob-field:field", 1);

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto intFc = traceCls.createSignedIntegerFieldClass();

            intFc->fieldValueRange(4);
            return singleMemberStructFc(traceCls, *intFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_integer_signed_set_value(firstMember(libStructFieldPtr), 1000);
        },
        "field-integer-signed-set-value:valid-value-for-field-class-field-value-range");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto intFc = traceCls.createUnsignedIntegerFieldClass();

            intFc->fieldValueRange(4);
            return singleMemberStructFc(traceCls, *intFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_integer_unsigned_set_value(firstMember(libStructFieldPtr), 1000);
        },
        "field-integer-unsigned-set-value:valid-value-for-field-class-field-value-range");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls,
                                        *traceCls.createStaticArrayFieldClass(
                                            *traceCls.createUnsignedIntegerFieldClass(), 2));
        },
        [](const auto libStructFieldPtr) {
            bt_field_array_borrow_element_field_by_index(firstMember(libStructFieldPtr), 99);
        },
        "field-array-borrow-element-field-by-index:valid-index");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls,
                                        *traceCls.createStaticArrayFieldClass(
                                            *traceCls.createUnsignedIntegerFieldClass(), 2));
        },
        [](const auto libStructFieldPtr) {
            bt_field_array_borrow_element_field_by_index_const(firstMember(libStructFieldPtr), 99);
        },
        "field-array-borrow-element-field-by-index-const:valid-index");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_structure_borrow_member_field_by_index(libStructFieldPtr, 99);
        },
        "field-structure-borrow-member-field-by-index:valid-index");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_structure_borrow_member_field_by_index_const(libStructFieldPtr, 99);
        },
        "field-structure-borrow-member-field-by-index-const:valid-index");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto varFc = traceCls.createVariantFieldClass();

            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass());
            return singleMemberStructFc(traceCls, *varFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_variant_select_option_by_index(firstMember(libStructFieldPtr), 99);
        },
        "field-variant-select-option-by-index:valid-index");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto varFc = traceCls.createVariantFieldClass();

            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass());
            return singleMemberStructFc(traceCls, *varFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_variant_borrow_selected_option_field(firstMember(libStructFieldPtr));
        },
        "field-variant-borrow-selected-option-field:has-selected-field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto varFc = traceCls.createVariantFieldClass();

            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass());
            return singleMemberStructFc(traceCls, *varFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_variant_borrow_selected_option_field_const(firstMember(libStructFieldPtr));
        },
        "field-variant-borrow-selected-option-field-const:has-selected-field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto varFc = traceCls.createVariantFieldClass();

            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass());
            return singleMemberStructFc(traceCls, *varFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_variant_borrow_selected_option_class_const(firstMember(libStructFieldPtr));
        },
        "field-variant-borrow-selected-option-class-const:has-selected-field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto selFc = traceCls.createUnsignedIntegerFieldClass();
            const auto varFc = traceCls.createVariantWithUnsignedIntegerSelectorFieldClass(*selFc);
            const auto ranges = bt2::UnsignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass(), *ranges);

            const auto fc = traceCls.createStructureFieldClass();

            fc->appendMember("sel", *selFc);
            fc->appendMember("f", *varFc);
            return fc;
        },
        [](const auto libStructFieldPtr) {
            bt_field_variant_with_selector_field_integer_unsigned_borrow_selected_option_class_const(
                bt_field_structure_borrow_member_field_by_index(libStructFieldPtr, 1));
        },
        "field-variant-with-selector-field-integer-unsigned-borrow-selected-option-class-const:has-selected-field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto selFc = traceCls.createSignedIntegerFieldClass();
            const auto varFc = traceCls.createVariantWithSignedIntegerSelectorFieldClass(*selFc);
            const auto ranges = bt2::SignedIntegerRangeSet::create();

            ranges->addRange(0, 0);
            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass(), *ranges);

            const auto fc = traceCls.createStructureFieldClass();

            fc->appendMember("sel", *selFc);
            fc->appendMember("f", *varFc);
            return fc;
        },
        [](const auto libStructFieldPtr) {
            const auto varField =
                bt_field_structure_borrow_member_field_by_index(libStructFieldPtr, 1);

            bt_field_variant_with_selector_field_integer_signed_borrow_selected_option_class_const(
                varField);
        },
        "field-variant-with-selector-field-integer-signed-borrow-selected-option-class-const:has-selected-field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto varFc = traceCls.createVariantFieldClass();

            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass());
            return singleMemberStructFc(traceCls, *varFc);
        },
        [](const auto libStructFieldPtr) {
            bt_field_variant_get_selected_option_index(firstMember(libStructFieldPtr));
        },
        "field-variant-get-selected-option-index:has-selected-field");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStringFieldClass());
        },
        [](const auto libStructFieldPtr) {
            bt_field_string_append_with_length(firstMember(libStructFieldPtr), nullptr, 5);
        },
        "field-string-append-with-length:value-non-null-when-length-non-zero");

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStringFieldClass());
        },
        [](const auto libStructFieldPtr) {
            static const char value[] = {'a', '\0', 'b', 'c'};

            bt_field_string_append_with_length(firstMember(libStructFieldPtr), value,
                                               sizeof(value));
        },
        "field-string-append-with-length:value-has-no-null-byte");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBoolFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_bool_set_value(libFieldPtr, BT_TRUE);
        },
        "field-bool-set-value:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createBitArrayFieldClass(8);
        },
        [](const auto libFieldPtr) {
            bt_field_bit_array_set_value_as_integer(libFieldPtr, 0);
        },
        "field-bit-array-set-value-as-integer:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createSignedIntegerFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_integer_signed_set_value(libFieldPtr, 0);
        },
        "field-integer-signed-set-value:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createUnsignedIntegerFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_integer_unsigned_set_value(libFieldPtr, 0);
        },
        "field-integer-unsigned-set-value:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createSinglePrecisionRealFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_real_single_precision_set_value(libFieldPtr, 0.0f);
        },
        "field-real-single-precision-set-value:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createDoublePrecisionRealFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_real_double_precision_set_value(libFieldPtr, 0.0);
        },
        "field-real-double-precision-set-value:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStringFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_string_set_value(libFieldPtr, "x");
        },
        "field-string-set-value:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStringFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_string_append_with_length(libFieldPtr, "x", 1);
        },
        "field-string-append-with-length:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStringFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_string_append(libFieldPtr, "x");
        },
        "field-string-append:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStringFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_string_clear(libFieldPtr);
        },
        "field-string-clear:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createDynamicArrayFieldClass(
                *traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libFieldPtr) {
            bt_field_array_dynamic_set_length(libFieldPtr, 0);
        },
        "field-array-dynamic-set-length:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createOptionFieldClass(*traceCls.createUnsignedIntegerFieldClass());
        },
        [](const auto libFieldPtr) {
            bt_field_option_set_has_field(libFieldPtr, BT_FALSE);
        },
        "field-option-set-has-field:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            const auto varFc = traceCls.createVariantFieldClass();

            varFc->appendOption("a", *traceCls.createUnsignedIntegerFieldClass());
            return varFc;
        },
        [](const auto libFieldPtr) {
            bt_field_variant_select_option_by_index(libFieldPtr, 0);
        },
        "field-variant-select-option-by-index:not-frozen:field");

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createDynamicBlobWithoutLengthFieldLocationFieldClass();
        },
        [](const auto libFieldPtr) {
            bt_field_blob_dynamic_set_length(libFieldPtr, 0);
        },
        "field-blob-dynamic-set-length:not-frozen:field", 1);

    addNotFrozenFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return traceCls.createStaticBlobFieldClass(4);
        },
        [](const auto libFieldPtr) {
            bt_field_blob_get_data(libFieldPtr);
        },
        "field-blob-get-data:not-frozen:field", 1);

    addEventPayloadFieldTrigger(
        triggers,
        [](const auto traceCls) {
            return singleMemberStructFc(traceCls, *traceCls.createStaticBlobFieldClass(0));
        },
        [](const auto libStructFieldPtr) {
            bt_field_blob_get_data(firstMember(libStructFieldPtr));
        },
        "field-blob-get-data:blob-field-length-is-set", 1);
}
