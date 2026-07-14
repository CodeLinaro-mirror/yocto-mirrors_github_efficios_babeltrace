/*
 * Copyright (c) 2026 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_PLUGIN_PROVIDER_HPP
#define BABELTRACE_CPP_COMMON_BT2_PLUGIN_PROVIDER_HPP

#include <optional>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2c/c-string-view.hpp"

#include "borrowed-object.hpp"
#include "version.hpp"

namespace bt2 {

class ConstPluginProvider final : public BorrowedObject<const bt_plugin_provider>
{
public:
    explicit ConstPluginProvider(const LibObjPtr libObjPtr) noexcept
        : _ThisBorrowedObject {libObjPtr}
    {
    }

    bt2c::CStringView name() const noexcept
    {
        return bt_plugin_provider_get_name(this->libObjPtr());
    }

    bt2c::CStringView description() const noexcept
    {
        return bt_plugin_provider_get_description(this->libObjPtr());
    }

    bt2c::CStringView author() const noexcept
    {
        return bt_plugin_provider_get_author(this->libObjPtr());
    }

    bt2c::CStringView license() const noexcept
    {
        return bt_plugin_provider_get_license(this->libObjPtr());
    }

    bt2c::CStringView path() const noexcept
    {
        return bt_plugin_provider_get_path(this->libObjPtr());
    }

    std::optional<Version> version() const noexcept
    {
        unsigned int major, minor, patch;
        const char *extra;

        if (bt_plugin_provider_get_version(this->libObjPtr(), &major, &minor, &patch, &extra) ==
            BT_PROPERTY_AVAILABILITY_NOT_AVAILABLE) {
            return std::nullopt;
        }

        return Version {major, minor, patch, extra};
    }
};

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_PLUGIN_PROVIDER_HPP */
