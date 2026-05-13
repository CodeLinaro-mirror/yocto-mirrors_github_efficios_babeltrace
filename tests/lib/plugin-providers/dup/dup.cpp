/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

/* A shared object with two plugin providers sharing the same name. */

#include <cstdio>

#include <babeltrace2/babeltrace.h>

namespace {

bt_plugin_provider_initialize_func_status providerInit(bt_self_plugin_provider *)
{
    std::printf("init test-provider-dup\n");
    return BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_STATUS_OK;
}

void providerFinalize(bt_self_plugin_provider *)
{
    std::printf("exit test-provider-dup\n");
}

} /* namespace */

BT_PLUGIN_PROVIDER_MODULE();

BT_PLUGIN_PROVIDER_WITH_ID(provider_one, "test-provider-dup");
BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_WITH_ID(provider_one, providerInit);
BT_PLUGIN_PROVIDER_FINALIZE_FUNC_WITH_ID(provider_one, providerFinalize);

BT_PLUGIN_PROVIDER_WITH_ID(provider_two, "test-provider-dup");
BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_WITH_ID(provider_two, providerInit);
BT_PLUGIN_PROVIDER_FINALIZE_FUNC_WITH_ID(provider_two, providerFinalize);
