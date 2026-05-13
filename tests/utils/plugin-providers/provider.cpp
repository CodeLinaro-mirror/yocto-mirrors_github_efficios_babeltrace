/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <babeltrace2/babeltrace.h>

BT_PLUGIN_PROVIDER_MODULE();

BT_PLUGIN_PROVIDER_WITH_ID(provider_one, "test-provider-1");
BT_PLUGIN_PROVIDER_DESCRIPTION_WITH_ID(provider_one, "Provider one");
BT_PLUGIN_PROVIDER_AUTHOR_WITH_ID(provider_one, "Test Author");
BT_PLUGIN_PROVIDER_LICENSE_WITH_ID(provider_one, "MIT");
BT_PLUGIN_PROVIDER_VERSION_WITH_ID(provider_one, 1, 2, 3, "dev");

BT_PLUGIN_PROVIDER_WITH_ID(provider_two, "test-provider-2");
BT_PLUGIN_PROVIDER_DESCRIPTION_WITH_ID(provider_two, "Provider two");
BT_PLUGIN_PROVIDER_AUTHOR_WITH_ID(provider_two, "Another Author");
BT_PLUGIN_PROVIDER_LICENSE_WITH_ID(provider_two, "GPLv2");
BT_PLUGIN_PROVIDER_VERSION_WITH_ID(provider_two, 4, 5, 6, nullptr);

BT_PLUGIN_PROVIDER_WITH_ID(provider_three, "test-provider-3");
