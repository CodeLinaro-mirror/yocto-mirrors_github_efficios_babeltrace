/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS Inc.
 */

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/field-location.hpp"
#include "cpp-common/bt2s/span.hpp"

#include "conds-triggers.hpp"
#include "utils.hpp"

/*
 * Adds field location API precondition failure triggers.
 */
void addFieldLocPreCondsTriggers(CondTriggers& triggers)
{
    triggers.emplace_back(makeRunInCompInitTrigger(
        [](const auto selfComp) {
            const char * const items[] = {"field"};

            bt_field_location_create(selfComp.createTraceClass()->libObjPtr(),
                                     BT_FIELD_LOCATION_SCOPE_PACKET_CONTEXT, items, 1);
        },
        CondTrigger::Type::Pre, "field-location-create:mip-version-is-valid", 0));

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            bt_field_location_create(selfComp.createTraceClass()->libObjPtr(),
                                     BT_FIELD_LOCATION_SCOPE_PACKET_CONTEXT, nullptr, 0);
        },
        "field-location-create:item-count-ge-1");

    addPreTrigger(
        triggers,
        [] {
            bt_field_location_get_root_scope(nullptr);
        },
        "field-location-get-root-scope:not-null:field-location");

    addPreTrigger(
        triggers,
        [] {
            bt_field_location_get_item_count(nullptr);
        },
        "field-location-get-item-count:not-null:field-location");

    addPreTrigger(
        triggers,
        [] {
            bt_field_location_get_item_by_index(nullptr, 0);
        },
        "field-location-get-item-by-index:not-null:field-location");

    addRunInCompInitTriggerPerMipVersion(
        triggers,
        [](const auto selfComp) {
            const char * const items[] = {"field"};
            const auto fieldLoc = selfComp.createTraceClass()->createFieldLocation(
                bt2::ConstFieldLocation::Scope::PacketContext,
                bt2s::span<const char * const> {items, 1});

            bt_field_location_get_item_by_index(fieldLoc->libObjPtr(), 1);
        },
        "field-location-get-item-by-index:valid-index");
}
