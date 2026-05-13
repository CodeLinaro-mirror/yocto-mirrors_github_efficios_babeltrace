/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

/*
 * Helper program which forces libbabeltrace2 to load its plugin
 * providers, then prints the name of each loaded plugin provider, one
 * per line, with the format `provider NAME`.
 */

#include <cstdint>

#include <fmt/core.h>

#include <babeltrace2/babeltrace.h>

int main()
{
    const bt_plugin_provider_set *provider_set;

    if (const auto status = bt_plugin_provider_set_borrow(&provider_set);
        status != BT_PLUGIN_PROVIDER_SET_BORROW_STATUS_OK) {
        fmt::print(stderr, "Failed to borrow plugin provider set: status={}\n",
                   static_cast<int>(status));
        return 1;
    }

    for (std::uint64_t i = 0; i < bt_plugin_provider_set_get_plugin_provider_count(provider_set);
         ++i) {
        fmt::print("provider {}\n",
                   bt_plugin_provider_get_name(
                       bt_plugin_provider_set_borrow_plugin_provider_by_index(provider_set, i)));
    }
}
