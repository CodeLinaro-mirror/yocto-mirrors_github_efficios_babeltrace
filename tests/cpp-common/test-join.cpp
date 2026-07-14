/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <string>
#include <string_view>
#include <vector>

#include "cpp-common/bt2c/join.hpp"

#include "catch2/catch_test_macros.hpp"

TEST_CASE("bt2c::join() with an empty container")
{
    const std::vector<std::string> container;

    CHECK(bt2c::join(container, ", ") == "");
}

TEST_CASE("bt2c::join() with a single element")
{
    const std::vector<std::string> container {"allo"};

    CHECK(bt2c::join(container, ", ") == "allo");
}

TEST_CASE("bt2c::join() with two or more elements")
{
    const std::vector<std::string> container {"allo", "bobo", "rhume", "cerveau"};

    CHECK(bt2c::join(container, ", ") == "allo, bobo, rhume, cerveau");
}

TEST_CASE("bt2c::join() with an empty delimiter")
{
    const std::vector<std::string> container {"allo", "bobo"};

    CHECK(bt2c::join(container, "") == "allobobo");
}

TEST_CASE("bt2c::join() with a multi-character delimiter")
{
    const std::vector<std::string> container {"allo", "bobo", "rhume"};

    CHECK(bt2c::join(container, " -> ") == "allo -> bobo -> rhume");
}

TEST_CASE("bt2c::join() with `std::string_view` elements")
{
    const std::vector<std::string_view> container {"allo", "bobo"};

    CHECK(bt2c::join(container, ", ") == "allo, bobo");
}
