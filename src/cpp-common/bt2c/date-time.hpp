/*
 * Copyright (c) 2026 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef BABELTRACE_CPP_COMMON_BT2C_DATE_TIME_HPP
#define BABELTRACE_CPP_COMMON_BT2C_DATE_TIME_HPP

#include "common/assert.h"

namespace bt2c {

/*!
@brief
    Calendar date.

@ingroup common-cpp-bt2c

A date is a year, a month, and a day of month of the proleptic
Gregorian calendar.

A date object is always valid: build one with Date::Date() after
checking its parts with isValid().

@code{.cpp}
#include "cpp-common/bt2c/date-time.hpp"
@endcode
*/
class Date final
{
public:
    /*!
    @brief
        Returns whether or not the year \bt_p{year}, the month
        \bt_p{month}, and the day of month \bt_p{day} make a valid date.

    \bt_p{month} must be within [1,&nbsp;12] and \bt_p{day} must be
    within [1,&nbsp;<em>N</em>], where <em>N</em> is the number of days
    of the month \bt_p{month} of the year \bt_p{year} (honoring leap
    years).

    @param[in] year
        Year.
    @param[in] month
        Month (1 is January).
    @param[in] day
        Day of month (1 is the first day).

    @returns
        \c true if \bt_p{year}, \bt_p{month}, and \bt_p{day} make a
        valid date.
    */
    static bool isValid(int year, unsigned int month, unsigned int day) noexcept;

    /*!
    @brief
        Builds a date having the year \bt_p{year}, the month
        \bt_p{month}, and the day of month \bt_p{day}.

    @param[in] year
        Year.
    @param[in] month
        Month (1 is January).
    @param[in] day
        Day of month (1 is the first day).

    @pre
        isValid() returns \c true given the same parameters.
    */
    explicit Date(const int year, const unsigned int month, const unsigned int day) noexcept
        : _mYear {year},
          _mMonth {month},
          _mDay {day}
    {
        BT_ASSERT_DBG(isValid(year, month, day));
    }

    /*!
    @brief
        Year.

    @returns
        Year.
    */
    int year() const noexcept
    {
        return _mYear;
    }

    /*!
    @brief
        Month (1 is January).

    @returns
        Month.
    */
    unsigned int month() const noexcept
    {
        return _mMonth;
    }

    /*!
    @brief
        Day of month (1 is the first day).

    @returns
        Day of month.
    */
    unsigned int day() const noexcept
    {
        return _mDay;
    }

    /*!
    @brief
        Returns whether or not this date is equal to \bt_p{other}.

    @param[in] other
        Other date to compare to.

    @returns
        \c true if this date is equal to \bt_p{other}.
    */
    bool operator==(const Date& other) const noexcept
    {
        return _mYear == other._mYear && _mMonth == other._mMonth && _mDay == other._mDay;
    }

    /*!
    @brief
        Returns whether or not this date is not equal to \bt_p{other}.

    @param[in] other
        Other date to compare to.

    @returns
        \c true if this date is not equal to \bt_p{other}.
    */
    bool operator!=(const Date& other) const noexcept
    {
        return !(*this == other);
    }

private:
    int _mYear;
    unsigned int _mMonth;
    unsigned int _mDay;
};

/*!
@brief
    Time of day.

@ingroup common-cpp-bt2c

A time of day is an hour, a minute, and a second.

A time of day object is always valid: build one with
TimeOfDay::TimeOfDay() after checking its parts with isValid().

@code{.cpp}
#include "cpp-common/bt2c/date-time.hpp"
@endcode
*/
class TimeOfDay final
{
public:
    /*!
    @brief
        Returns whether or not the hour \bt_p{hour}, the minute
        \bt_p{minute}, and the second \bt_p{second} make a valid
        time of day.

    \bt_p{hour} must be within [0,&nbsp;23], \bt_p{minute} within
    [0,&nbsp;59], and \bt_p{second} within [0,&nbsp;60] (60 being a
    leap second).

    @param[in] hour
        Hour.
    @param[in] minute
        Minute.
    @param[in] second
        Second.

    @returns
        \c true if \bt_p{hour}, \bt_p{minute}, and \bt_p{second} make a
        valid time of day.
    */
    static bool isValid(const unsigned int hour, const unsigned int minute,
                        const unsigned int second) noexcept
    {
        return hour <= 23 && minute <= 59 && second <= 60;
    }

    /*!
    @brief
        Builds a time of day having the hour \bt_p{hour}, the minute
        \bt_p{minute}, and the second \bt_p{second}.

    @param[in] hour
        Hour.
    @param[in] minute
        Minute.
    @param[in] second
        Second (60 is a leap second).

    @pre
        isValid() returns \c true given the same parameters.
    */
    explicit TimeOfDay(const unsigned int hour = 0, const unsigned int minute = 0,
                       const unsigned int second = 0) noexcept
        : _mHour {hour},
          _mMinute {minute},
          _mSecond {second}
    {
        BT_ASSERT_DBG(isValid(hour, minute, second));
    }

    /*!
    @brief
        Hour.

    @returns
        Hour.
    */
    unsigned int hour() const noexcept
    {
        return _mHour;
    }

    /*!
    @brief
        Minute.

    @returns
        Minute.
    */
    unsigned int minute() const noexcept
    {
        return _mMinute;
    }

    /*!
    @brief
        Second (60 is a leap second).

    @returns
        Second.
    */
    unsigned int second() const noexcept
    {
        return _mSecond;
    }

    /*!
    @brief
        Returns whether or not this time of day is equal to
        \bt_p{other}.

    @param[in] other
        Other time of day to compare to.

    @returns
        \c true if this time of day is equal to \bt_p{other}.
    */
    bool operator==(const TimeOfDay& other) const noexcept
    {
        return _mHour == other._mHour && _mMinute == other._mMinute && _mSecond == other._mSecond;
    }

    /*!
    @brief
        Returns whether or not this time of day is not equal to
        \bt_p{other}.

    @param[in] other
        Other time of day to compare to.

    @returns
        \c true if this time of day is not equal to \bt_p{other}.
    */
    bool operator!=(const TimeOfDay& other) const noexcept
    {
        return !(*this == other);
    }

private:
    unsigned int _mHour;
    unsigned int _mMinute;
    unsigned int _mSecond;
};

/*!
@brief
    Date and time of day.

@ingroup common-cpp-bt2c

A date/time is a \link Date date\endlink and a
\link TimeOfDay time of day\endlink, \em without any time zone.

A date/time object is always valid because its parts are too.

@code{.cpp}
#include "cpp-common/bt2c/date-time.hpp"
@endcode
*/
class DateTime final
{
public:
    /*!
    @brief
        Builds a date/time having the date \bt_p{date} and the time of
        day \bt_p{timeOfDay}.

    @param[in] date
        Date.
    @param[in] timeOfDay
        Time of day.
    */
    explicit DateTime(const Date& date, const TimeOfDay& timeOfDay = TimeOfDay {}) noexcept
        : _mDate {date},
          _mTimeOfDay {timeOfDay}
    {
    }

    /*!
    @brief
        Builds a date/time having the date
        \bt_p{year}/\bt_p{month}/\bt_p{day} and the time of day
        \bt_p{hour}:\bt_p{minute}:\bt_p{second}.

    @param[in] year
        Year.
    @param[in] month
        Month (1 is January).
    @param[in] day
        Day of month (1 is the first day).
    @param[in] hour
        Hour.
    @param[in] minute
        Minute.
    @param[in] second
        Second (60 is a leap second).

    @pre
        Date::isValid() returns \c true given \bt_p{year},
        \bt_p{month}, and \bt_p{day}.
    @pre
        TimeOfDay::isValid() returns \c true given \bt_p{hour},
        \bt_p{minute}, and \bt_p{second}.
    */
    explicit DateTime(const int year, const unsigned int month, const unsigned int day,
                      const unsigned int hour = 0, const unsigned int minute = 0,
                      const unsigned int second = 0) noexcept
        : _mDate {year, month, day},
          _mTimeOfDay {hour, minute, second}
    {
    }

    /*!
    @brief
        Date.

    @returns
        Date.
    */
    const Date& date() const noexcept
    {
        return _mDate;
    }

    /*!
    @brief
        Time of day.

    @returns
        Time of day.
    */
    const TimeOfDay& timeOfDay() const noexcept
    {
        return _mTimeOfDay;
    }

    /*!
    @brief
        Returns whether or not this date/time is equal to
        \bt_p{other}.

    @param[in] other
        Other date/time to compare to.

    @returns
        \c true if this date/time is equal to \bt_p{other}.
    */
    bool operator==(const DateTime& other) const noexcept
    {
        return _mDate == other._mDate && _mTimeOfDay == other._mTimeOfDay;
    }

    /*!
    @brief
        Returns whether or not this date/time is not equal to
        \bt_p{other}.

    @param[in] other
        Other date/time to compare to.

    @returns
        \c true if this date/time is not equal to \bt_p{other}.
    */
    bool operator!=(const DateTime& other) const noexcept
    {
        return !(*this == other);
    }

private:
    Date _mDate;
    TimeOfDay _mTimeOfDay;
};

} /* namespace bt2c */

#endif /* BABELTRACE_CPP_COMMON_BT2C_DATE_TIME_HPP */
