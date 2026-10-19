/*
 * Copyright (c) 2026 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <algorithm>
#include <array>
#include <charconv>
#include <cstring>
#include <limits>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>

#include <glib.h>

#include "common/assert.h"
#include "common/common.h"
#include "compat/time.h"
#include "compat/utc.h"
#include "cpp-common/bt2c/exc.hpp"
#include "cpp-common/bt2c/regex.hpp"
#include "cpp-common/bt2c/safe-ops.hpp"

#include "tz.hpp"

/*
 * g_time_zone_new_identifier() exists since GLib 2.68, but the build
 * may restrict the available GLib API to an older version.
 */
#if GLIB_CHECK_VERSION(2, 68, 0)
#    if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_68
#        define BT2C_TZ_HAVE_G_TIME_ZONE_NEW_IDENTIFIER
#    endif
#endif

/* Likewise for g_time_zone_get_identifier(), which exists since GLib 2.58 */
#if GLIB_CHECK_VERSION(2, 58, 0)
#    if GLIB_VERSION_MAX_ALLOWED >= GLIB_VERSION_2_58
#        define BT2C_TZ_HAVE_G_TIME_ZONE_GET_IDENTIFIER
#    endif
#endif

namespace bt2c {
namespace {

/*
 * Returns the GLib time zone `id`, or `nullptr` if GLib can't find it.
 */
GTimeZoneUP gTzFromId(const CStringView id)
{
#ifdef BT2C_TZ_HAVE_G_TIME_ZONE_NEW_IDENTIFIER
    return GTimeZoneUP {g_time_zone_new_identifier(id)};
#else
    /*
     * Before GLib 2.68, g_time_zone_new() returns the UTC time zone
     * when it can't find or parse `id`.
     */
    G_GNUC_BEGIN_IGNORE_DEPRECATIONS
    GTimeZoneUP tz {g_time_zone_new(id)};
    G_GNUC_END_IGNORE_DEPRECATIONS

#    ifdef BT2C_TZ_HAVE_G_TIME_ZONE_GET_IDENTIFIER
    /*
     * Detect this case: the identifier of the UTC fallback time zone
     * is `UTC`, whereas the identifier of a time zone which GLib finds
     * or parses (for example, the POSIX `TZ` string `UTC0`) is `id`.
     */
    if (id != "UTC" && std::strcmp(g_time_zone_get_identifier(tz.get()), "UTC") == 0) {
        return nullptr;
    }
#    else
    /*
     * Detect this case: the UTC fallback time zone has a single
     * interval with the `UTC` abbreviation and a zero offset. Accept
     * such a time zone only if `id` is a well-known name of UTC.
     *
     * This also rejects a POSIX `TZ` string which GLib parses as such
     * a time zone, for example `UTC0`: without
     * g_time_zone_get_identifier(), it's indistinguishable from
     * the fallback.
     */
    if (g_time_zone_get_offset(tz.get(), 0) == 0 &&
        std::strcmp(g_time_zone_get_abbreviation(tz.get(), 0), "UTC") == 0) {
        static constexpr std::array<const char *, 8> utcIds = {
            "UTC", "Etc/UTC", "UCT", "Etc/UCT", "Universal", "Etc/Universal", "Zulu", "Etc/Zulu"};

        if (std::none_of(utcIds.begin(), utcIds.end(), [id](const char * const utcId) {
                return id == utcId;
            })) {
            return nullptr;
        }
    }
#    endif

    return tz;
#endif
}

/*
 * Returns the value of the decimal digits `str`.
 */
unsigned int decVal(const std::string_view str) noexcept
{
    unsigned int val = 0;
    const auto res = std::from_chars(str.data(), str.data() + str.size(), val);

    BT_ASSERT_DBG(res.ec == std::errc {});
    BT_ASSERT_DBG(res.ptr == str.data() + str.size());
    return val;
}

/*
 * Returns the date/time of the broken-down time `tm`, or `std::nullopt`
 * if its year doesn't fit in an `int`.
 */
std::optional<DateTime> dateTimeFromTm(const std::tm& tm) noexcept
{
    if (tm.tm_year > std::numeric_limits<int>::max() - 1900) {
        return std::nullopt;
    }

    return DateTime {tm.tm_year + 1900,
                     static_cast<unsigned int>(tm.tm_mon + 1),
                     static_cast<unsigned int>(tm.tm_mday),
                     static_cast<unsigned int>(tm.tm_hour),
                     static_cast<unsigned int>(tm.tm_min),
                     static_cast<unsigned int>(tm.tm_sec)};
}

/*
 * Returns the broken-down time of `dateTime`, ready to pass to
 * mktime() or timegm(), or `std::nullopt` if its year doesn't fit in
 * the `tm_year` member.
 */
std::optional<std::tm> tmFromDateTime(const DateTime& dateTime) noexcept
{
    const auto& date = dateTime.date();
    const auto& timeOfDay = dateTime.timeOfDay();

    if (date.year() < std::numeric_limits<int>::min() + 1900) {
        return std::nullopt;
    }

    std::tm tm {};

    tm.tm_year = date.year() - 1900;
    tm.tm_mon = static_cast<int>(date.month()) - 1;
    tm.tm_mday = static_cast<int>(date.day());
    tm.tm_hour = static_cast<int>(timeOfDay.hour());
    tm.tm_min = static_cast<int>(timeOfDay.minute());
    tm.tm_sec = static_cast<int>(timeOfDay.second());

    /* Let mktime() determine whether or not DST is in effect */
    tm.tm_isdst = -1;
    return tm;
}

/* A UTC offset may be negative */
static_assert(std::is_signed_v<std::time_t>, "`std::time_t` is a signed type.");

} /* namespace */

Tz::Tz(const Kind kind, const int offsetSec, GTimeZoneUP gTz) noexcept
    : _mKind {kind},
      _mOffsetSec {offsetSec},
      _mGTz {std::move(gTz)}
{
}

std::optional<Tz> Tz::createNamed(const CStringView id)
{
    if (auto gTz = gTzFromId(id)) {
        return Tz {Kind::Named, 0, std::move(gTz)};
    }

    return std::nullopt;
}

Tz Tz::createFromStr(const CStringView str, const Logger& logger)
{
    if (str.len() == 0) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW_SPEC(logger, Error, "Empty time zone string.");
    }

    if (str == "local") {
        return Tz::createLocal();
    } else if (str == "utc") {
        return Tz::createUtc();
    } else if (str[0] == '+' || str[0] == '-') {
        /* `±HH:MM` */
        static const Regex utcOffsetRegex {"^[+-]([0-9]{2}):([0-9]{2})\\z"};

        const auto match = utcOffsetRegex.matchGroups(str);
        const auto hours = match ? decVal(*(*match)[1]) : 0U;
        const auto mins = match ? decVal(*(*match)[2]) : 0U;

        if (!match || hours > 23 || mins > 59) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW_SPEC(
                logger, Error,
                "Invalid UTC offset `{}`: expecting `+HH:MM` or `-HH:MM`, where `HH` is within "
                "[`00`, `23`] and `MM` is within [`00`, `59`].",
                str);
        }

        if (hours == 0 && mins == 0 && str[0] == '-') {
            /* RFC 3339, 4.3: `-00:00` means "unknown local offset" */
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW_SPEC(
                logger, Error, "Invalid UTC offset `-00:00`: use `+00:00` or `utc`.");
        }

        const auto absOffsetSec = static_cast<int>(hours * 3600 + mins * 60);

        return Tz::createFixedOffset(str[0] == '-' ? -absOffsetSec : absOffsetSec);
    }

    /* Time zone identifier */
    if (auto tz = Tz::createNamed(str)) {
        return std::move(*tz);
    }

    BT_CPPLOGE_APPEND_CAUSE_AND_THROW_SPEC(logger, Error, "Unknown time zone `{}`.", str);
}

std::optional<int> Tz::utcOffsetSec(const std::time_t sec) const noexcept
{
    switch (_mKind) {
    case Kind::Local:
    {
        std::tm tm;

        if (!bt_localtime_r(&sec, &tm)) {
            return std::nullopt;
        }

        /*
         * The difference between the broken-down local time,
         * interpreted as UTC, and the original time is the UTC offset
         * in effect at that time (honors daylight saving time).
         */
        const auto localAsUtcSec = bt_timegm(&tm);

        if (localAsUtcSec == -1) {
            return std::nullopt;
        }

        return static_cast<int>(localAsUtcSec - sec);
    }

    case Kind::Utc:
        return 0;

    case Kind::FixedOffset:
        return _mOffsetSec;

    case Kind::Named:
        return g_time_zone_get_offset(_mGTz.get(),
                                      g_time_zone_find_interval(_mGTz.get(), G_TIME_TYPE_UNIVERSAL,
                                                                static_cast<gint64>(sec)));
    }

    bt_common_abort();
}

std::optional<DateTime> Tz::dateTime(const std::time_t sec) const noexcept
{
    std::tm tm;

    switch (_mKind) {
    case Kind::Local:
        if (!bt_localtime_r(&sec, &tm)) {
            return std::nullopt;
        }

        return dateTimeFromTm(tm);

    case Kind::Utc:
        if (!bt_gmtime_r(&sec, &tm)) {
            return std::nullopt;
        }

        return dateTimeFromTm(tm);

    case Kind::FixedOffset:
    case Kind::Named:
    {
        /*
         * Broken-down UTC time of the time shifted by the UTC offset in
         * effect at `sec`.
         */
        const auto offsetSec = this->utcOffsetSec(sec);

        BT_ASSERT_DBG(offsetSec);

        const auto localSec = bt2c::safeAdd<std::time_t>(sec, *offsetSec);

        if (!localSec || !bt_gmtime_r(&*localSec, &tm)) {
            return std::nullopt;
        }

        return dateTimeFromTm(tm);
    }
    }

    bt_common_abort();
}

std::optional<std::time_t> Tz::secFromDateTime(const DateTime& dateTime) const noexcept
{
    auto tm = tmFromDateTime(dateTime);

    if (!tm) {
        return std::nullopt;
    }

    switch (_mKind) {
    case Kind::Local:
    {
        const auto sec = std::mktime(&*tm);

        if (sec == -1) {
            return std::nullopt;
        }

        return sec;
    }

    case Kind::Utc:
    {
        const auto sec = bt_timegm(&*tm);

        if (sec == -1) {
            return std::nullopt;
        }

        return sec;
    }

    case Kind::FixedOffset:
    case Kind::Named:
    {
        /* Date/time in this time zone, as if it was UTC */
        const auto utcSec = bt_timegm(&*tm);

        if (utcSec == -1) {
            return std::nullopt;
        }

        if (_mKind == Kind::FixedOffset) {
            return bt2c::safeSub<std::time_t>(utcSec, _mOffsetSec);
        }

        /*
         * When the UTC offset decreases (for example, when DST ends),
         * the same date/time occurs twice: g_time_zone_adjust_time()
         * picks the occurrence of which the interval has the DST flag
         * of the requested type. The first occurrence isn't always the
         * DST one: some time zones (for example, `Europe/Dublin`) flag
         * their winter time as DST (negative DST).
         *
         * Therefore, adjust with both `G_TIME_TYPE_STANDARD` and
         * `G_TIME_TYPE_DAYLIGHT` and keep the earliest time, that is,
         * the first occurrence.
         *
         * When the UTC offset increases (for example, when DST starts),
         * some date/times don't exist: g_time_zone_adjust_time() moves
         * them forward to the start of the next interval, whatever
         * the type.
         */
        const auto secFromType = [this,
                                  utcSec](const GTimeType type) -> std::optional<std::time_t> {
            auto localSec = static_cast<gint64>(utcSec);
            const auto interval = g_time_zone_adjust_time(_mGTz.get(), type, &localSec);

            /* g_time_zone_adjust_time() may move `localSec` forward */
            if (localSec < std::numeric_limits<std::time_t>::min() ||
                localSec > std::numeric_limits<std::time_t>::max()) {
                return std::nullopt;
            }

            return bt2c::safeSub<std::time_t>(static_cast<std::time_t>(localSec),
                                              g_time_zone_get_offset(_mGTz.get(), interval));
        };

        const auto stdSec = secFromType(G_TIME_TYPE_STANDARD);
        const auto dstSec = secFromType(G_TIME_TYPE_DAYLIGHT);

        if (!stdSec || !dstSec) {
            return std::nullopt;
        }

        return std::min(*stdSec, *dstSec);
    }
    }

    bt_common_abort();
}

} /* namespace bt2c */
