/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#ifndef BABELTRACE_STRING_FORMAT_FORMAT_PLUGIN_COMP_CLS_NAME_HPP
#define BABELTRACE_STRING_FORMAT_FORMAT_PLUGIN_COMP_CLS_NAME_HPP

#include <string>

#include <babeltrace2/babeltrace.h>

#include "common/common.h"
#include "cpp-common/bt2/component-class.hpp"

/*
 * Returns the printable representation of a component class.
 *
 * `pluginName` is optional; pass an empty string view if the component class
 * does not come from a plugin.
 */
std::string formatPluginCompClsOpt(bt2c::CStringView pluginName, bt2c::CStringView compClsName,
                                   bt2::ComponentClassType type, bt_common_color_when useColors);

#endif /* BABELTRACE_STRING_FORMAT_FORMAT_PLUGIN_COMP_CLS_NAME_HPP */
