/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

/*
 * A shared object with two plugin providers, one that fails to init and one
 * that fails.
 */

#include <cstdio>
#include <cstdlib>

#include <babeltrace2/babeltrace.h>

namespace {

bt_plugin_provider_initialize_func_status providerFailInit(bt_self_plugin_provider *)
{
    std::printf("init test-provider-init-fail\n");
    return BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_STATUS_ERROR;
}

void providerFailFinalize(bt_self_plugin_provider *)
{
    /* This should never be called. */
    std::abort();
}

bt_plugin_provider_initialize_func_status providerOkInit(bt_self_plugin_provider *)
{
    std::printf("init test-provider-init-ok\n");
    return BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_STATUS_OK;
}

void providerOkFinalize(bt_self_plugin_provider *)
{
    std::printf("exit test-provider-init-ok\n");
}

} /* namespace */

BT_PLUGIN_PROVIDER_MODULE();

BT_PLUGIN_PROVIDER_WITH_ID(provider_fail, "test-provider-init-fail");
BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_WITH_ID(provider_fail, providerFailInit);
BT_PLUGIN_PROVIDER_FINALIZE_FUNC_WITH_ID(provider_fail, providerFailFinalize);

BT_PLUGIN_PROVIDER_WITH_ID(provider_ok, "test-provider-init-ok");
BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_WITH_ID(provider_ok, providerOkInit);
BT_PLUGIN_PROVIDER_FINALIZE_FUNC_WITH_ID(provider_ok, providerOkFinalize);
