/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

/* A shared object containing two plugin providers. */

#include <cstdio>

#include <babeltrace2/babeltrace.h>

namespace {

bt_plugin_provider_initialize_func_status providerOneInit(bt_self_plugin_provider *)
{
    std::printf("init test-provider-one\n");
    return BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_STATUS_OK;
}

void providerOneFinalize(bt_self_plugin_provider *)
{
    std::printf("exit test-provider-one\n");
}

bt_plugin_provider_initialize_func_status providerTwoInit(bt_self_plugin_provider *)
{
    std::printf("init test-provider-two\n");
    return BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_STATUS_OK;
}

void providerTwoFinalize(bt_self_plugin_provider *)
{
    std::printf("exit test-provider-two\n");
}

} /* namespace */

BT_PLUGIN_PROVIDER_MODULE();

BT_PLUGIN_PROVIDER_WITH_ID(provider_one, "test-provider-one");
BT_PLUGIN_PROVIDER_DESCRIPTION_WITH_ID(provider_one,
                                       "First provider in a multi-provider shared object");
BT_PLUGIN_PROVIDER_AUTHOR_WITH_ID(provider_one, "EfficiOS");
BT_PLUGIN_PROVIDER_LICENSE_WITH_ID(provider_one, "MIT");
BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_WITH_ID(provider_one, providerOneInit);
BT_PLUGIN_PROVIDER_FINALIZE_FUNC_WITH_ID(provider_one, providerOneFinalize);

BT_PLUGIN_PROVIDER_WITH_ID(provider_two, "test-provider-two");
BT_PLUGIN_PROVIDER_DESCRIPTION_WITH_ID(provider_two,
                                       "Second provider in a multi-provider shared object");
BT_PLUGIN_PROVIDER_AUTHOR_WITH_ID(provider_two, "EfficiOS");
BT_PLUGIN_PROVIDER_LICENSE_WITH_ID(provider_two, "MIT");
BT_PLUGIN_PROVIDER_INITIALIZE_FUNC_WITH_ID(provider_two, providerTwoInit);
BT_PLUGIN_PROVIDER_FINALIZE_FUNC_WITH_ID(provider_two, providerTwoFinalize);
