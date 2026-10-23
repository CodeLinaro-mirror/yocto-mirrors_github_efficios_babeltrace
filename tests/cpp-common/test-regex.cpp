/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <string_view>

#include "cpp-common/bt2c/regex.hpp"

#include "catch2/catch_test_macros.hpp"

TEST_CASE("bt2c::Regex::match()")
{
    const bt2c::Regex regex {"^[0-9]{2}:[0-9]{2}$"};

    CHECK(regex.match("12:34"));
    CHECK_FALSE(regex.match("12:345"));
    CHECK_FALSE(regex.match("1:34"));
    CHECK_FALSE(regex.match(""));
}

TEST_CASE("bt2c::Regex::match() with a default-constructed string view")
{
    CHECK(bt2c::Regex {"^$"}.match(std::string_view {}));
    CHECK_FALSE(bt2c::Regex {"a"}.match(std::string_view {}));
}

TEST_CASE("bt2c::Regex::matchGroups() without a match")
{
    const bt2c::Regex regex {"^([0-9]+)$"};

    CHECK_FALSE(regex.matchGroups("12a"));
    CHECK_FALSE(regex.matchGroups(""));
    CHECK_FALSE(regex.matchGroups(std::string_view {}));
}

TEST_CASE("bt2c::Regex::matchGroups() without capturing groups")
{
    const auto match = bt2c::Regex {"b(?:c|d)"}.matchGroups("abde");

    REQUIRE(match);
    REQUIRE(match->count() == 1);
    CHECK((*match)[0] == "bd");
}

TEST_CASE("bt2c::Regex::matchGroups() with capturing groups")
{
    const std::string_view str {"2026-10-23"};
    const auto match = bt2c::Regex {"^([0-9]{4})-([0-9]{2})-([0-9]{2})$"}.matchGroups(str);

    REQUIRE(match);
    REQUIRE(match->count() == 4);
    CHECK((*match)[0] == "2026-10-23");
    CHECK((*match)[1] == "2026");
    CHECK((*match)[2] == "10");
    CHECK((*match)[3] == "23");

    /* Views on the original string */
    CHECK((*match)[0]->data() == str.data());
    CHECK((*match)[3]->data() == str.data() + 8);
}

TEST_CASE("bt2c::Regex::matchGroups() with an empty participating group")
{
    const auto match = bt2c::Regex {"^(-?)([0-9]+)$"}.matchGroups("42");

    REQUIRE(match);
    REQUIRE(match->count() == 3);
    REQUIRE((*match)[1]);
    CHECK((*match)[1]->empty());
    CHECK((*match)[2] == "42");
}

TEST_CASE("bt2c::Regex::matchGroups() with non-participating groups")
{
    const bt2c::Regex regex {"^(?:(a)|(b))([0-9]{2})(?::([0-9]{2})(?:\\.([0-9]+))?)?$"};

    SECTION("Non-taken alternative and trailing optional groups")
    {
        const auto match = regex.matchGroups("b12");

        REQUIRE(match);
        REQUIRE(match->count() == 6);
        CHECK_FALSE((*match)[1]);
        CHECK((*match)[2] == "b");
        CHECK((*match)[3] == "12");
        CHECK_FALSE((*match)[4]);
        CHECK_FALSE((*match)[5]);
    }

    SECTION("Only the innermost optional group doesn't participate")
    {
        const auto match = regex.matchGroups("a12:34");

        REQUIRE(match);
        REQUIRE(match->count() == 6);
        CHECK((*match)[1] == "a");
        CHECK_FALSE((*match)[2]);
        CHECK((*match)[3] == "12");
        CHECK((*match)[4] == "34");
        CHECK_FALSE((*match)[5]);
    }

    SECTION("All groups but the non-taken alternative participate")
    {
        const auto match = regex.matchGroups("a12:34.5");

        REQUIRE(match);
        CHECK((*match)[4] == "34");
        CHECK((*match)[5] == "5");
    }
}

TEST_CASE("bt2c::Regex::matchGroups() with a non-null-terminated string view")
{
    /* Only consider the first five characters */
    const std::string_view str {"12:34:56", 5};
    const auto match = bt2c::Regex {"^([0-9]{2}):([0-9]{2})$"}.matchGroups(str);

    REQUIRE(match);
    CHECK((*match)[1] == "12");
    CHECK((*match)[2] == "34");
}
