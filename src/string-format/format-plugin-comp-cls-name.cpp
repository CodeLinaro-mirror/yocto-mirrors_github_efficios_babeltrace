/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright EfficiOS, Inc.
 */

#include "cpp-common/bt2c/exc.hpp"
#include "cpp-common/bt2c/fmt.hpp" /* IWYU pragma: keep */
#include "cpp-common/bt2c/glib-up.hpp"

#include "format-plugin-comp-cls-name.hpp"

namespace {

const char *componentTypeStr(const bt2::ComponentClassType type)
{
    switch (type) {
    case bt2::ComponentClassType::Source:
        return "source";
    case bt2::ComponentClassType::Sink:
        return "sink";
    case bt2::ComponentClassType::Filter:
        return "filter";
    default:
        return "(unknown)";
    }
}

} /* namespace */

std::string formatPluginCompClsOpt(bt2c::CStringView pluginName, bt2c::CStringView compClsName,
                                   const bt2::ComponentClassType type,
                                   const bt_common_color_when useColors)
{
    const bt_common_color_codes codes = bt_common_color_get_codes(useColors);
    auto str = fmt::format("'{}{}{}{}", codes.bold, codes.fg_bright_cyan, componentTypeStr(type),
                           codes.fg_default);
    const auto shellPluginName = std::invoke([pluginName] {
        if (pluginName) {
            const auto quoted = bt_common_shell_quote(pluginName, false);

            if (!quoted) {
                throw bt2c::MemoryError {};
            }

            return bt2c::GStringUP {quoted};
        } else {
            return bt2c::GStringUP {};
        }
    });

    if (shellPluginName) {
        fmt::format_to(std::back_inserter(str), ".{}{}{}", codes.fg_blue, shellPluginName->str,
                       codes.fg_default);
    }

    const bt2c::GStringUP shellCompClsName {bt_common_shell_quote(compClsName, false)};

    if (!shellCompClsName) {
        throw bt2c::MemoryError {};
    }

    fmt::format_to(std::back_inserter(str), ".{}{}{}'", codes.fg_yellow, shellCompClsName->str,
                   codes.reset);

    return str;
}
