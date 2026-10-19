/*
 * Copyright (c) 2026 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_TZ_HPP
#define BABELTRACE_CPP_COMMON_BT2C_TZ_HPP

#include <ctime>
#include <optional>

#include "common/assert.h"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/date-time.hpp"
#include "cpp-common/bt2c/glib-up.hpp"
#include "cpp-common/bt2c/logging.hpp"

namespace bt2c {

/*!
@brief
    Time zone.

@ingroup common-cpp-bt2c

A time zone is one of:

<dl>
  <dt>Local time zone (Tz::Kind::Local)</dt>
  <dd>
    Local time zone of the system, as localtime() and mktime() see it.
  </dd>

  <dt>UTC (Tz::Kind::Utc)</dt>
  <dd>
    Coordinated Universal Time.
  </dd>

  <dt>Fixed UTC offset (Tz::Kind::FixedOffset)</dt>
  <dd>
    Fixed offset from UTC, without daylight saving time.
  </dd>

  <dt>Named time zone (Tz::Kind::Named)</dt>
  <dd>
    Time zone which a
    <a href="https://docs.gtk.org/glib/struct.TimeZone.html"><code>GTimeZone</code></a>
    identifier names, for example <code>America/Toronto</code>,
    honoring its own daylight saving time rules.
  </dd>
</dl>

Create a time zone with createLocal(), createUtc(), createFixedOffset(),
createNamed(), or createFromStr().

With a time zone, you may:

- Get the UTC offset in effect at some time with utcOffsetSec().

- Convert a time (seconds from the Unix epoch) to a date/time in this
  time zone with dateTime().

- Convert a date/time in this time zone to a time with
  secFromDateTime().

A time zone object is movable, but not copyable.

@code{.cpp}
#include "cpp-common/bt2c/tz.hpp"
@endcode
*/
class Tz final
{
public:
    /*!
    @brief
        Time zone kind.
    */
    enum class Kind
    {
        /*! Local time zone of the system. */
        Local,

        /*! UTC. */
        Utc,

        /*! Fixed UTC offset. */
        FixedOffset,

        /*! Named time zone. */
        Named,
    };

private:
    explicit Tz(Kind kind, int offsetSec = 0, GTimeZoneUP gTz = {}) noexcept;

public:
    /*!
    @brief
        Returns the local time zone of the system.

    @returns
        Local time zone.
    */
    static Tz createLocal() noexcept
    {
        return Tz {Kind::Local};
    }

    /*!
    @brief
        Returns UTC.

    @returns
        UTC time zone.
    */
    static Tz createUtc() noexcept
    {
        return Tz {Kind::Utc};
    }

    /*!
    @brief
        Returns the time zone having the fixed UTC offset
        \bt_p{offsetSec}.

    @param[in] offsetSec
        UTC offset (seconds), positive east of UTC.

    @pre
        The absolute value of \bt_p{offsetSec} is less than 86,400
        (one day).

    @returns
        Time zone having the fixed UTC offset \bt_p{offsetSec}.
    */
    static Tz createFixedOffset(const int offsetSec) noexcept
    {
        BT_ASSERT_DBG(offsetSec > -86400 && offsetSec < 86400);
        return Tz {Kind::FixedOffset, offsetSec};
    }

    /*!
    @brief
        Returns the named time zone \bt_p{id}, if GLib knows it.

    \bt_p{id} is a
    <a href="https://docs.gtk.org/glib/ctor.TimeZone.new_identifier.html"><code>GTimeZone</code> identifier</a>,
    for example <code>America/Toronto</code> or
    <code>EST5EDT,M3.2.0,M11.1.0</code>.

    @param[in] id
        Identifier of the time zone to get.

    @returns
        Named time zone \bt_p{id}, or \c std::nullopt if GLib can't
        find or parse it.
    */
    static std::optional<Tz> createNamed(CStringView id);

    /*!
    @brief
        Returns the time zone of which \bt_p{str} is the
        string representation.

    \bt_p{str} is one of:

    <dl>
      <dt><code>local</code></dt>
      <dd>Local time zone (see createLocal()).</dd>

      <dt><code>utc</code></dt>
      <dd>UTC (see createUtc()).</dd>

      <dt><code>+HH:MM</code></dt>
      <dt><code>-HH:MM</code></dt>
      <dd>
        Fixed UTC offset (see createFixedOffset()), where
        <code>HH</code> is within [<code>00</code>, <code>23</code>] and
        <code>MM</code> is within [<code>00</code>, <code>59</code>].

        <code>-00:00</code> is invalid: RFC&nbsp;3339 gives it the
        meaning of an unknown local offset.
      </dd>

      <dt>Anything else</dt>
      <dd>Time zone identifier (see createNamed()).</dd>
    </dl>

    @param[in] str
        String representation of the time zone to get.
    @param[in] logger
        Logger to use to append an error cause before throwing.

    @returns
        Time zone of which \bt_p{str} is the string representation.

    @throws Error
        @parblock
        \bt_p{str} is invalid.

        @note
            The appended error cause doesn't mention where \bt_p{str}
            comes from: catch the exception to append a cause with
            this context.
        @endparblock
    */
    static Tz createFromStr(CStringView str, const Logger& logger);

    /*!
    @brief
        Kind of this time zone.

    @returns
        Kind of this time zone.
    */
    Kind kind() const noexcept
    {
        return _mKind;
    }

    /*!
    @brief
        Returns the UTC offset of this time zone in effect at
        \bt_p{sec} (seconds from the Unix epoch).

    @param[in] sec
        Time at which to get the UTC offset (seconds from the
        Unix epoch).

    @returns
        @parblock
        UTC offset (seconds), positive east of UTC, in effect at
        \bt_p{sec}, or \c std::nullopt if an underlying timing call
        fails (only possible with the local time zone).

        The returned offset isn't necessarily a whole number of
        minutes, for example with some historical time zones.
        @endparblock
    */
    std::optional<int> utcOffsetSec(std::time_t sec) const noexcept;

    /*!
    @brief
        Returns the date/time of \bt_p{sec} (seconds from the Unix
        epoch) in this time zone.

    @param[in] sec
        Time to convert (seconds from the Unix epoch).

    @returns
        Date/time of \bt_p{sec} in this time zone, or \c std::nullopt
        if an underlying timing call fails, if \bt_p{sec} shifted by
        the UTC offset doesn't fit in \c std::time_t, or if the year
        doesn't fit in an \c int.

    @sa secFromDateTime()
    */
    std::optional<DateTime> dateTime(std::time_t sec) const noexcept;

    /*!
    @brief
        Returns the time (seconds from the Unix epoch) of the date/time
        \bt_p{dateTime} in this time zone.

    When \bt_p{dateTime} is ambiguous or doesn't exist because of a
    UTC offset transition, with a named time zone:

    - If \bt_p{dateTime} occurs twice (UTC offset decrease, for example
      at the end of daylight saving time), this method uses its
      first occurrence.

    - If \bt_p{dateTime} doesn't exist (UTC offset increase, for example
      at the start of daylight saving time), this method uses the time
      of this transition.

    With the local time zone, this behavior depends on mktime().

    This method treats a leap second (second 60) as the first second of
    the next minute, unless the local time zone of the system knows
    about leap seconds.

    @param[in] dateTime
        Date/time to convert.

    @returns
        Time (seconds from the Unix epoch) of \bt_p{dateTime} in this
        time zone, or \c std::nullopt if an underlying timing call
        fails, if the year is out of range for those calls, or if the
        resulting time doesn't fit in \c std::time_t.

    @sa dateTime()
    */
    std::optional<std::time_t> secFromDateTime(const DateTime& dateTime) const noexcept;

private:
    /* Kind of this time zone */
    Kind _mKind;

    /* UTC offset (seconds) when `_mKind` is `Kind::FixedOffset` */
    int _mOffsetSec;

    /* GLib time zone when `_mKind` is `Kind::Named` */
    GTimeZoneUP _mGTz;
};

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_TZ_HPP */
