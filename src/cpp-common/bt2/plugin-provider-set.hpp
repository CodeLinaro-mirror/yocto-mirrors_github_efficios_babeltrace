/*
 * Copyright (c) 2026 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2_PLUGIN_PROVIDER_SET_HPP
#define BABELTRACE_CPP_COMMON_BT2_PLUGIN_PROVIDER_SET_HPP

#include <cstdint>

#include <babeltrace2/babeltrace.h>

#include "common/common.h"
#include "cpp-common/bt2/borrowed-object-iterator.hpp"
#include "cpp-common/bt2/plugin-provider.hpp"

#include "borrowed-object.hpp"
#include "exc.hpp"

namespace bt2 {

class ConstPluginProviderSet final : public BorrowedObject<const bt_plugin_provider_set>
{
public:
    using Iterator = BorrowedObjectIterator<ConstPluginProviderSet>;

    explicit ConstPluginProviderSet(const LibObjPtr libObjPtr) noexcept
        : _ThisBorrowedObject {libObjPtr}
    {
    }

    std::uint64_t length() const noexcept
    {
        return bt_plugin_provider_set_get_plugin_provider_count(this->libObjPtr());
    }

    Iterator begin() const noexcept
    {
        return Iterator {*this, 0};
    }

    Iterator end() const noexcept
    {
        return Iterator {*this, this->length()};
    }

    ConstPluginProvider operator[](const std::uint64_t index) const noexcept
    {
        return ConstPluginProvider {
            bt_plugin_provider_set_borrow_plugin_provider_by_index(this->libObjPtr(), index)};
    }
};

inline ConstPluginProviderSet pluginProviderSet()
{
    const bt_plugin_provider_set *libObjPtr;

    switch (bt_plugin_provider_set_borrow(&libObjPtr)) {
    case BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_OK:
        return ConstPluginProviderSet {libObjPtr};
    case BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_MEMORY_ERROR:
        throw MemoryError {};
    case BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_ERROR:
        throw Error {};
    }

    bt_common_abort();
}

} /* namespace bt2 */

#endif /* BABELTRACE_CPP_COMMON_BT2_PLUGIN_PROVIDER_SET_HPP */
