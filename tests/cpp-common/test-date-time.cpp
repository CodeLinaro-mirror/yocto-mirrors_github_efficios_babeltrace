/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include "cpp-common/bt2c/date-time.hpp"
#include "cpp-common/bt2c/fmt.hpp" /* IWYU pragma: keep */

#include "catch2/catch_test_macros.hpp"

TEST_CASE("bt2c::Date::isValid() with valid dates")
{
    CHECK(bt2c::Date::isValid(2026, 1, 1));
    CHECK(bt2c::Date::isValid(2026, 1, 31));
    CHECK(bt2c::Date::isValid(2026, 4, 30));
    CHECK(bt2c::Date::isValid(2026, 12, 31));
    CHECK(bt2c::Date::isValid(1970, 1, 1));
    CHECK(bt2c::Date::isValid(0, 1, 1));
    CHECK(bt2c::Date::isValid(-44, 3, 15));
}

TEST_CASE("bt2c::Date::isValid() with an invalid month")
{
    CHECK_FALSE(bt2c::Date::isValid(2026, 0, 1));
    CHECK_FALSE(bt2c::Date::isValid(2026, 13, 1));
}

TEST_CASE("bt2c::Date::isValid() with an invalid day")
{
    CHECK_FALSE(bt2c::Date::isValid(2026, 1, 0));
    CHECK_FALSE(bt2c::Date::isValid(2026, 1, 32));
    CHECK_FALSE(bt2c::Date::isValid(2026, 4, 31));
    CHECK_FALSE(bt2c::Date::isValid(2026, 6, 31));
    CHECK_FALSE(bt2c::Date::isValid(2026, 9, 31));
    CHECK_FALSE(bt2c::Date::isValid(2026, 11, 31));
    CHECK_FALSE(bt2c::Date::isValid(2026, 2, 30));
}

TEST_CASE("bt2c::Date::isValid() with 29 February")
{
    SECTION("Leap years")
    {
        /* Divisible by 4, but not by 100 */
        CHECK(bt2c::Date::isValid(2024, 2, 29));

        /* Divisible by 400 */
        CHECK(bt2c::Date::isValid(2000, 2, 29));
        CHECK(bt2c::Date::isValid(0, 2, 29));
        CHECK(bt2c::Date::isValid(-400, 2, 29));

        /* Negative, divisible by 4, but not by 100 */
        CHECK(bt2c::Date::isValid(-4, 2, 29));
    }

    SECTION("Common years")
    {
        /* Not divisible by 4 */
        CHECK_FALSE(bt2c::Date::isValid(2026, 2, 29));
        CHECK_FALSE(bt2c::Date::isValid(-1, 2, 29));

        /* Divisible by 100, but not by 400 */
        CHECK_FALSE(bt2c::Date::isValid(1900, 2, 29));
        CHECK_FALSE(bt2c::Date::isValid(2100, 2, 29));
        CHECK_FALSE(bt2c::Date::isValid(-100, 2, 29));
    }

    SECTION("Never 30 February")
    {
        CHECK_FALSE(bt2c::Date::isValid(2024, 2, 30));
    }
}

TEST_CASE("`bt2c::Date` accessors")
{
    const bt2c::Date date {2026, 10, 23};

    CHECK(date.year() == 2026);
    CHECK(date.month() == 10);
    CHECK(date.day() == 23);
}

TEST_CASE("`bt2c::Date` equality")
{
    const bt2c::Date date {2026, 10, 23};

    CHECK(date == bt2c::Date {2026, 10, 23});
    CHECK_FALSE(date != bt2c::Date {2026, 10, 23});
    CHECK(date != bt2c::Date {2025, 10, 23});
    CHECK(date != bt2c::Date {2026, 11, 23});
    CHECK(date != bt2c::Date {2026, 10, 24});
}

TEST_CASE("bt2c::TimeOfDay::isValid()")
{
    SECTION("Valid")
    {
        CHECK(bt2c::TimeOfDay::isValid(0, 0, 0));
        CHECK(bt2c::TimeOfDay::isValid(12, 34, 56));
        CHECK(bt2c::TimeOfDay::isValid(23, 59, 59));

        /* Leap second */
        CHECK(bt2c::TimeOfDay::isValid(23, 59, 60));
    }

    SECTION("Invalid")
    {
        CHECK_FALSE(bt2c::TimeOfDay::isValid(24, 0, 0));
        CHECK_FALSE(bt2c::TimeOfDay::isValid(0, 60, 0));
        CHECK_FALSE(bt2c::TimeOfDay::isValid(0, 0, 61));
    }
}

TEST_CASE("`bt2c::TimeOfDay` accessors")
{
    SECTION("Default")
    {
        const bt2c::TimeOfDay timeOfDay;

        CHECK(timeOfDay.hour() == 0);
        CHECK(timeOfDay.minute() == 0);
        CHECK(timeOfDay.second() == 0);
    }

    SECTION("Explicit")
    {
        const bt2c::TimeOfDay timeOfDay {12, 34, 56};

        CHECK(timeOfDay.hour() == 12);
        CHECK(timeOfDay.minute() == 34);
        CHECK(timeOfDay.second() == 56);
    }
}

TEST_CASE("`bt2c::TimeOfDay` equality")
{
    const bt2c::TimeOfDay timeOfDay {12, 34, 56};

    CHECK(timeOfDay == bt2c::TimeOfDay {12, 34, 56});
    CHECK_FALSE(timeOfDay != bt2c::TimeOfDay {12, 34, 56});
    CHECK(timeOfDay != bt2c::TimeOfDay {13, 34, 56});
    CHECK(timeOfDay != bt2c::TimeOfDay {12, 35, 56});
    CHECK(timeOfDay != bt2c::TimeOfDay {12, 34, 57});
}

TEST_CASE("`bt2c::DateTime` accessors")
{
    SECTION("Default time of day")
    {
        const bt2c::DateTime dt {bt2c::Date {2026, 10, 23}};

        CHECK(dt.date() == bt2c::Date {2026, 10, 23});
        CHECK(dt.timeOfDay() == bt2c::TimeOfDay {});
    }

    SECTION("Explicit time of day")
    {
        const bt2c::DateTime dt {bt2c::Date {2026, 10, 23}, bt2c::TimeOfDay {12, 34, 56}};

        CHECK(dt.date() == bt2c::Date {2026, 10, 23});
        CHECK(dt.timeOfDay() == bt2c::TimeOfDay {12, 34, 56});
    }

    SECTION("Parts, default time of day")
    {
        const bt2c::DateTime dt {2026, 10, 23};

        CHECK(dt.date() == bt2c::Date {2026, 10, 23});
        CHECK(dt.timeOfDay() == bt2c::TimeOfDay {});
    }

    SECTION("Parts")
    {
        const bt2c::DateTime dt {2026, 10, 23, 12, 34, 56};

        CHECK(dt.date() == bt2c::Date {2026, 10, 23});
        CHECK(dt.timeOfDay() == bt2c::TimeOfDay {12, 34, 56});
    }
}

TEST_CASE("`bt2c::DateTime` equality")
{
    const bt2c::DateTime dt {bt2c::Date {2026, 10, 23}, bt2c::TimeOfDay {12, 34, 56}};

    CHECK(dt == bt2c::DateTime {bt2c::Date {2026, 10, 23}, bt2c::TimeOfDay {12, 34, 56}});
    CHECK_FALSE(dt != bt2c::DateTime {bt2c::Date {2026, 10, 23}, bt2c::TimeOfDay {12, 34, 56}});
    CHECK(dt != bt2c::DateTime {bt2c::Date {2026, 10, 24}, bt2c::TimeOfDay {12, 34, 56}});
    CHECK(dt != bt2c::DateTime {bt2c::Date {2026, 10, 23}, bt2c::TimeOfDay {12, 34, 57}});
}

TEST_CASE("`bt2c::Date` formatting")
{
    CHECK(fmt::format("{}", bt2c::Date {2026, 10, 23}) == "2026-10-23");
    CHECK(fmt::format("{}", bt2c::Date {1970, 1, 1}) == "1970-01-01");
    CHECK(fmt::format("{}", bt2c::Date {987, 6, 5}) == "0987-06-05");
    CHECK(fmt::format("{}", bt2c::Date {0, 1, 1}) == "0000-01-01");
    CHECK(fmt::format("{}", bt2c::Date {9999, 12, 31}) == "9999-12-31");

    /* ISO 8601 expanded representation */
    CHECK(fmt::format("{}", bt2c::Date {-44, 3, 15}) == "-0044-03-15");
    CHECK(fmt::format("{}", bt2c::Date {-12345, 3, 15}) == "-12345-03-15");
    CHECK(fmt::format("{}", bt2c::Date {10000, 1, 1}) == "+10000-01-01");
}

TEST_CASE("`bt2c::TimeOfDay` formatting")
{
    CHECK(fmt::format("{}", bt2c::TimeOfDay {}) == "00:00:00");
    CHECK(fmt::format("{}", bt2c::TimeOfDay {1, 2, 3}) == "01:02:03");
    CHECK(fmt::format("{}", bt2c::TimeOfDay {23, 59, 60}) == "23:59:60");
}

TEST_CASE("`bt2c::DateTime` formatting")
{
    CHECK(fmt::format("{}",
                      bt2c::DateTime {bt2c::Date {2026, 10, 23}, bt2c::TimeOfDay {14, 10, 33}}) ==
          "2026-10-23T14:10:33");
    CHECK(fmt::format("{}", bt2c::DateTime {bt2c::Date {-44, 3, 15}}) == "-0044-03-15T00:00:00");
}
