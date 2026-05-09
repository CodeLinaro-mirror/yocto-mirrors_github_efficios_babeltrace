/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2017 Philippe Proulx <pproulx@efficios.com>
 * Copyright (C) 2024 EfficiOS, Inc.
 */

#include <functional>

#include <fmt/core.h>
#include <glib.h>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2/exc.hpp"
#include "cpp-common/bt2/plugin-load.hpp"
#include "cpp-common/bt2c/c-string-view.hpp"

#include "catch2/catch_test_macros.hpp"

#define NON_EXISTING_PATH "/this/hopefully/does/not/exist/5bc75f8d-0dba-4043-a509-d7984b97e42b.so"

TEST_CASE("bt2::findAllPluginsFromDir() with nonexistent path")
{
    CHECK(std::invoke([] {
        try {
            bt2::findAllPluginsFromDir(NON_EXISTING_PATH, BT_FALSE, BT_FALSE);
            return false;
        } catch (const bt2::Error&) {
            bt_current_thread_clear_error();
            return true;
        }
    }));
}

TEST_CASE("bt2::findAllPluginsFromDir() with valid path")
{
    const auto plugins = bt2::findAllPluginsFromDir(PLUGINS_DIR, BT_FALSE, BT_FALSE);

    REQUIRE(plugins);

    /* 2 or 4, if `.la` files are considered or not */
    CHECK((plugins->length() == 2 || plugins->length() == 4));
}

TEST_CASE("bt2::findPlugin() with unknown plugin name")
{
    CHECK_FALSE(bt2::findPlugin(NON_EXISTING_PATH, true, false, false, false, false));
}

TEST_CASE("bt2::findPlugin() finds a plugin using `BABELTRACE_PLUGIN_PATH`")
{
    g_setenv("BABELTRACE_PLUGIN_PATH",
             fmt::format("{}" G_SEARCHPATH_SEPARATOR_S G_DIR_SEPARATOR_S
                         "ec1d09e5-696c-442e-b1c3-f9c6cf7f5958" G_SEARCHPATH_SEPARATOR_S
                             G_SEARCHPATH_SEPARATOR_S G_SEARCHPATH_SEPARATOR_S
                         "{}" G_SEARCHPATH_SEPARATOR_S
                         "8db46494-a398-466a-9649-c765ae077629" G_SEARCHPATH_SEPARATOR_S,
                         NON_EXISTING_PATH, PLUGINS_DIR)
                 .c_str(),
             1);

    const auto plugin = bt2::findPlugin("test_minimal", true, false, false, false, false);

    REQUIRE(plugin);
    CHECK(plugin->author() == "Janine Sutto");
}

TEST_CASE("bt2::findAllPluginsFromDir() fails on load error")
{
    REQUIRE_THROWS_AS(bt2::findAllPluginsFromDir(INIT_FAIL_PLUGIN_DIR, false, true), bt2::Error);

    const auto error = bt_current_thread_take_error();

    REQUIRE(error);

    /*
     * The last error cause must be the one which the initialization
     * function of our plugin appended.
     */
    CHECK(bt2c::CStringView {bt_error_cause_get_message(
              bt_error_borrow_cause_by_index(error, 0))} == "This is the error message");
    bt_error_release(error);
}

TEST_CASE("bt2::findAllPluginsFromDir() doesn't fail on load error")
{
    CHECK(!bt2::findAllPluginsFromDir(INIT_FAIL_PLUGIN_DIR, false, false));
}
