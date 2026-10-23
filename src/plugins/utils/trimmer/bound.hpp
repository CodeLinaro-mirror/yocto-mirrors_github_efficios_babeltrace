/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2019-2026 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_UTILS_TRIMMER_BOUND_HPP
#define BABELTRACE_PLUGINS_UTILS_TRIMMER_BOUND_HPP

#include <cstdint>
#include <variant>

#include "common/assert.h"
#include "cpp-common/bt2c/date-time.hpp"

namespace bt2trimmer {

/*
 * Time of day (without a date) with nanoseconds.
 */
struct TimeOfDayWithNs final
{
    bt2c::TimeOfDay timeOfDay {};
    unsigned int ns = 0;
};

/*
 * Bound of a trimming time range, one of:
 *
 * Infinite:
 *     Negative or positive infinity, depending on which bound it is.
 *
 * Full time:
 *     Nanoseconds from origin.
 *
 * Time of day:
 *     Time without a date: a message iterator converts it to a full
 *     time once it knows the date of its first message having a time.
 */
class Bound final
{
private:
    struct _Infinite final
    {
    };

public:
    /*
     * Builds an infinite bound.
     */
    explicit Bound() noexcept = default;

    /*
     * Builds a full time bound of `nsFromOrig` nanoseconds
     * from origin.
     */
    explicit Bound(const std::int64_t nsFromOrig) noexcept
        : _mVal {nsFromOrig}
    {
    }

    /*
     * Builds a time of day bound.
     */
    explicit Bound(const TimeOfDayWithNs& timeOfDay) noexcept
        : _mVal {timeOfDay}
    {
    }

    /*
     * Whether or not this bound is infinite.
     */
    bool isInfinite() const noexcept
    {
        return std::holds_alternative<_Infinite>(_mVal);
    }

    /*
     * Whether or not this bound is a time of day which needs a date to
     * become a full time.
     */
    bool needsDate() const noexcept
    {
        return std::holds_alternative<TimeOfDayWithNs>(_mVal);
    }

    /*
     * Nanoseconds from origin of this full time bound.
     */
    std::int64_t nsFromOrig() const noexcept
    {
        const auto nsFromOrig = std::get_if<std::int64_t>(&_mVal);

        BT_ASSERT_DBG(nsFromOrig);
        return *nsFromOrig;
    }

    /*
     * Time of day of this time of day bound.
     */
    const TimeOfDayWithNs& timeOfDay() const noexcept
    {
        const auto timeOfDay = std::get_if<TimeOfDayWithNs>(&_mVal);

        BT_ASSERT_DBG(timeOfDay);
        return *timeOfDay;
    }

    /*
     * Whether or not this bound, as the end of a trimming time range,
     * is before the time `nsFromOrig`.
     *
     * An infinite end bound is never before any time.
     *
     * This bound must not be a time of day.
     */
    bool isBefore(const std::int64_t nsFromOrig) const noexcept
    {
        BT_ASSERT_DBG(!this->needsDate());
        return !this->isInfinite() && nsFromOrig > this->nsFromOrig();
    }

private:
    std::variant<_Infinite, std::int64_t, TimeOfDayWithNs> _mVal;
};

} /* namespace bt2trimmer */

#endif /* BABELTRACE_PLUGINS_UTILS_TRIMMER_BOUND_HPP */
