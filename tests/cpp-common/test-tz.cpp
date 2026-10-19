/*
 * SPDX-License-Identifier: GPL-2.0-only
 *
 * Copyright (C) 2026 EfficiOS, Inc.
 */

#include <cstdint>
#include <ctime>
#include <limits>
#include <optional>
#include <string>
#include <utility>

#include <glib.h>

#include <babeltrace2/babeltrace.h>

#include "cpp-common/bt2c/date-time.hpp"
#include "cpp-common/bt2c/exc.hpp"
#include "cpp-common/bt2c/logging.hpp"
#include "cpp-common/bt2c/tz.hpp"

#include "catch2/catch_test_macros.hpp"

namespace {

const bt2c::Logger theLogger {"test-tz", "TEST-TZ", bt2c::Logger::Level::None};

/*
 * Checks that `dt` has the date `year`/`month`/`day` and the time of
 * day `hour`/`minute`/`second`.
 */
void checkDt(const bt2c::DateTime& dt, const int year, const unsigned int month,
             const unsigned int day, const unsigned int hour, const unsigned int minute,
             const unsigned int second)
{
    CHECK(dt.date().year() == year);
    CHECK(dt.date().month() == month);
    CHECK(dt.date().day() == day);
    CHECK(dt.timeOfDay().hour() == hour);
    CHECK(dt.timeOfDay().minute() == minute);
    CHECK(dt.timeOfDay().second() == second);
}

/*
 * Returns the named time zone `id`, requiring that GLib knows it.
 */
bt2c::Tz namedTz(const char * const id)
{
    auto tz = bt2c::Tz::createNamed(id);

    REQUIRE(tz);
    return std::move(*tz);
}

/*
 * Checks that bt2c::Tz::createFromStr() throws `bt2c::Error` with
 * `str`, then clears the error of the current thread.
 */
void checkCreateFromStrThrows(const char * const str)
{
    INFO("String: `" << str << "`");
    CHECK_THROWS_AS(bt2c::Tz::createFromStr(str, theLogger), bt2c::Error);
    bt_current_thread_clear_error();
}

/*
 * Sets the `TZ` environment variable to some value during its lifetime,
 * restoring its original value on destruction.
 */
class TzEnvGuard final
{
public:
    explicit TzEnvGuard(const char * const val)
    {
        if (const auto origVal = g_getenv("TZ")) {
            _mOrigVal = origVal;
        }

        g_setenv("TZ", val, TRUE);
        tzset();
    }

    TzEnvGuard(const TzEnvGuard&) = delete;
    TzEnvGuard& operator=(const TzEnvGuard&) = delete;

    ~TzEnvGuard()
    {
        if (_mOrigVal) {
            g_setenv("TZ", _mOrigVal->c_str(), TRUE);
        } else {
            g_unsetenv("TZ");
        }

        tzset();
    }

private:
    std::optional<std::string> _mOrigVal;
};

} /* namespace */

/*
 * POSIX `TZ` string of the US Eastern time zone (EST/EDT, DST from the
 * second Sunday of March to the first Sunday of November).
 *
 * GLib parses such a string itself: using it doesn't require the time
 * zone database of the system.
 */
constexpr const char *easternTzStr = "EST5EDT,M3.2.0,M11.1.0";

TEST_CASE("bt2c::Tz::kind()")
{
    CHECK(bt2c::Tz::createLocal().kind() == bt2c::Tz::Kind::Local);
    CHECK(bt2c::Tz::createUtc().kind() == bt2c::Tz::Kind::Utc);
    CHECK(bt2c::Tz::createFixedOffset(3600).kind() == bt2c::Tz::Kind::FixedOffset);
    CHECK(namedTz(easternTzStr).kind() == bt2c::Tz::Kind::Named);
}

TEST_CASE("bt2c::Tz::createNamed() with an unknown identifier")
{
    CHECK_FALSE(bt2c::Tz::createNamed("Mars/Olympus_Mons"));
}

TEST_CASE("bt2c::Tz::createFromStr() with `local` and `utc`")
{
    CHECK(bt2c::Tz::createFromStr("local", theLogger).kind() == bt2c::Tz::Kind::Local);
    CHECK(bt2c::Tz::createFromStr("utc", theLogger).kind() == bt2c::Tz::Kind::Utc);
}

TEST_CASE("bt2c::Tz::createFromStr() with a UTC offset")
{
    const auto check = [](const char * const str, const int expectedOffsetSec) {
        INFO("String: `" << str << "`");

        const auto tz = bt2c::Tz::createFromStr(str, theLogger);

        CHECK(tz.kind() == bt2c::Tz::Kind::FixedOffset);
        CHECK(tz.utcOffsetSec(0) == expectedOffsetSec);
    };

    check("+00:00", 0);
    check("+05:30", 5 * 3600 + 30 * 60);
    check("-04:00", -4 * 3600);
    check("+23:59", 23 * 3600 + 59 * 60);
    check("-23:59", -(23 * 3600 + 59 * 60));
}

/* 2026-01-15T12:00:00Z (EST) */
constexpr std::time_t winterSec = 1768478400;

TEST_CASE("bt2c::Tz::createFromStr() with a time zone identifier")
{
    const auto tz = bt2c::Tz::createFromStr(easternTzStr, theLogger);

    CHECK(tz.kind() == bt2c::Tz::Kind::Named);
    CHECK(tz.utcOffsetSec(winterSec) == -5 * 3600);
}

TEST_CASE("bt2c::Tz::createFromStr() with an invalid string")
{
    /* Empty */
    checkCreateFromStrThrows("");

    /* Invalid UTC offsets */
    for (const auto str : {"+", "-", "+5:00", "-0400", "+05:3", "+05:300", "+24:00", "-05:60",
                           "+0a:00", "+05-30", "+05:30\n", "-00:00"}) {
        checkCreateFromStrThrows(str);
    }

    /* Unknown time zone identifier */
    checkCreateFromStrThrows("Mars/Olympus_Mons");

    /* Case-sensitive keywords */
    checkCreateFromStrThrows("UTC-ish");
    checkCreateFromStrThrows("Local");
}

/* 2026-07-15T12:00:00Z (EDT) */
constexpr std::time_t summerSec = 1784116800;

/*
 * 2040-07-15T12:00:00Z (EDT), which doesn't fit a signed 32-bit integer.
 *
 * Only use it when `std::time_t` is a 64-bit integer (see
 * `timeTIs64Bit`).
 */
constexpr std::int64_t summer2040Sec = 2225966400;

/* Whether or not `std::time_t` is a 64-bit integer */
constexpr bool timeTIs64Bit = sizeof(std::time_t) >= 8;

TEST_CASE("`bt2c::Tz` with UTC")
{
    const auto tz = bt2c::Tz::createUtc();

    CHECK(tz.utcOffsetSec(summerSec) == 0);

    const auto dt = tz.dateTime(summerSec);

    REQUIRE(dt);
    checkDt(*dt, 2026, 7, 15, 12, 0, 0);
    CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 7, 15, 12}) == summerSec);
    CHECK(tz.secFromDateTime(bt2c::DateTime {1970, 1, 1}) == 0);

    /* Leap second: first second of the next minute */
    CHECK(tz.secFromDateTime(bt2c::DateTime {2016, 12, 31, 23, 59, 60}) == 1483228800);

    /* Year out of range for timegm() */
    CHECK_FALSE(tz.secFromDateTime(bt2c::DateTime {std::numeric_limits<int>::min(), 1, 1}));

    if (timeTIs64Bit) {
        /* After 2038 */
        CHECK(tz.secFromDateTime(bt2c::DateTime {2040, 7, 15, 12}) == summer2040Sec);

        /* Greatest second of which the nanoseconds fit `std::int64_t` */
        CHECK(tz.secFromDateTime(bt2c::DateTime {2262, 4, 11, 23, 47, 16}) == 9223372036);

#ifndef __MINGW32__
        /*
         * Before the Unix epoch.
         *
         * On MinGW, bt_timegm() calls _mkgmtime(), which fails with a
         * date before 1970.
         */
        CHECK(tz.secFromDateTime(bt2c::DateTime {1900, 1, 1}) == -2208988800);
#endif
    }
}

TEST_CASE("`bt2c::Tz` with a fixed UTC offset")
{
    const auto tz = bt2c::Tz::createFixedOffset(5 * 3600 + 30 * 60);
    const auto dt = tz.dateTime(0);

    REQUIRE(dt);
    checkDt(*dt, 1970, 1, 1, 5, 30, 0);
    CHECK(tz.secFromDateTime(bt2c::DateTime {1970, 1, 1, 5, 30}) == 0);

    /* No DST */
    CHECK(tz.utcOffsetSec(winterSec) == 5 * 3600 + 30 * 60);
    CHECK(tz.utcOffsetSec(summerSec) == 5 * 3600 + 30 * 60);

    if (timeTIs64Bit) {
        /* After 2038 */
        CHECK(tz.secFromDateTime(bt2c::DateTime {2040, 7, 15, 17, 30}) == summer2040Sec);
    }
}

TEST_CASE("`bt2c::Tz` with a named time zone")
{
    const auto tz = namedTz(easternTzStr);

    SECTION("UTC offset follows DST")
    {
        CHECK(tz.utcOffsetSec(winterSec) == -5 * 3600);
        CHECK(tz.utcOffsetSec(summerSec) == -4 * 3600);
    }

    SECTION("Date/time")
    {
        auto dt = tz.dateTime(winterSec);

        REQUIRE(dt);
        checkDt(*dt, 2026, 1, 15, 7, 0, 0);

        dt = tz.dateTime(summerSec);
        REQUIRE(dt);
        checkDt(*dt, 2026, 7, 15, 8, 0, 0);
    }

    SECTION("Round trip")
    {
        CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 1, 15, 7}) == winterSec);
        CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 7, 15, 8}) == summerSec);
    }

    SECTION("After 2038")
    {
        if (timeTIs64Bit) {
            CHECK(tz.utcOffsetSec(static_cast<std::time_t>(summer2040Sec)) == -4 * 3600);
            CHECK(tz.secFromDateTime(bt2c::DateTime {2040, 7, 15, 8}) == summer2040Sec);
        }
    }

    SECTION("Date/time which occurs twice (end of DST): first occurrence")
    {
        /* 2026-11-01 01:30 EDT, that is, 2026-11-01T05:30:00Z */
        CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 11, 1, 1, 30}) == 1793511000);
    }

    SECTION("Date/time which doesn't exist (start of DST): start of DST")
    {
        /* 2026-03-08 03:00 EDT, that is, 2026-03-08T07:00:00Z */
        CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 3, 8, 2, 30}) == 1772953200);
    }
}

TEST_CASE("`bt2c::Tz` with a named time zone having negative DST")
{
    /*
     * The time zone database flags the winter time (GMT) of
     * `Europe/Dublin` as DST, and its summer time (IST) as standard
     * time. A POSIX `TZ` string can't express this: only run this test
     * if the system knows `Europe/Dublin`.
     */
    const auto tz = bt2c::Tz::createNamed("Europe/Dublin");

    if (!tz) {
        SKIP("Unknown `Europe/Dublin` time zone");
    }

    SECTION("Date/time which occurs twice (end of IST): first occurrence")
    {
        /* 2026-10-25 01:30 IST, that is, 2026-10-25T00:30:00Z */
        CHECK(tz->secFromDateTime(bt2c::DateTime {2026, 10, 25, 1, 30}) == 1792888200);
    }

    SECTION("Date/time which doesn't exist (start of IST): start of IST")
    {
        /* 2026-03-29 02:00 IST, that is, 2026-03-29T01:00:00Z */
        CHECK(tz->secFromDateTime(bt2c::DateTime {2026, 3, 29, 1, 30}) == 1774746000);
    }
}

TEST_CASE("`bt2c::Tz` with the local time zone")
{
    const TzEnvGuard tzEnvGuard {easternTzStr};
    const auto tz = bt2c::Tz::createLocal();

    SECTION("UTC offset follows DST")
    {
        CHECK(tz.utcOffsetSec(winterSec) == -5 * 3600);
        CHECK(tz.utcOffsetSec(summerSec) == -4 * 3600);
    }

    SECTION("Date/time")
    {
        auto dt = tz.dateTime(winterSec);

        REQUIRE(dt);
        checkDt(*dt, 2026, 1, 15, 7, 0, 0);

        dt = tz.dateTime(summerSec);
        REQUIRE(dt);
        checkDt(*dt, 2026, 7, 15, 8, 0, 0);
    }

    SECTION("Round trip")
    {
        CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 1, 15, 7}) == winterSec);
        CHECK(tz.secFromDateTime(bt2c::DateTime {2026, 7, 15, 8}) == summerSec);
    }

    SECTION("After 2038")
    {
        if (timeTIs64Bit) {
            CHECK(tz.utcOffsetSec(static_cast<std::time_t>(summer2040Sec)) == -4 * 3600);
            CHECK(tz.secFromDateTime(bt2c::DateTime {2040, 7, 15, 8}) == summer2040Sec);
        }
    }
}
