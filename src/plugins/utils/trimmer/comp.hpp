/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2019-2026 Philippe Proulx <pproulx@efficios.com>
 */

#ifndef BABELTRACE_PLUGINS_UTILS_TRIMMER_COMP_HPP
#define BABELTRACE_PLUGINS_UTILS_TRIMMER_COMP_HPP

#include <cstdint>
#include <optional>

#include "cpp-common/bt2/component-class-dev.hpp"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/date-time.hpp"
#include "cpp-common/bt2c/regex.hpp"
#include "cpp-common/bt2c/tz.hpp"

#include "bound.hpp"
#include "msg-iter.hpp"

namespace bt2trimmer {

/*
 * Expected format of the `begin` and `end` parameters (`type` entry of
 * the `ts-format` parameter).
 */
enum class TsFmtType
{
    /* Any format */
    Auto,

    /* `YYYY-MM-DD[ HH:II[:SS[.NANO]]]` or `HH:II[:SS[.NANO]]` */
    DateTime,

    /* `[-]SEC[.NANO]` */
    SecFromOrigin,

    /* `YYYY-MM-DDTHH:II:SS[.NANO]OFFSET` */
    Rfc3339,
};

class Comp final : public bt2::UserFilterComponent<Comp, MsgIter>
{
    friend class MsgIter;
    friend bt2::UserFilterComponent<Comp, MsgIter>;

public:
    explicit Comp(bt2::SelfFilterComponent selfComp, bt2::ConstMapValue params, void *);

protected:
    static void _getSupportedMipVersions(bt2::SelfComponentClass, bt2::ConstValue, void *,
                                         bt2::LoggingLevel, bt2::UnsignedIntegerRangeSet ranges);

private:
    /*
     * Returns the bound of which the parameter value `param` (signed
     * integer or string) is the representation.
     *
     * Throws `bt2c::Error` if `param` is invalid.
     */
    Bound _boundFromParam(bt2::ConstValue param) const;

    /*
     * Returns the bound of which `str` is the string representation.
     *
     * Throws `bt2c::Error` if `str` is invalid, including if its format
     * isn't the one of `_mTsFmtType`.
     */
    Bound _boundFromStr(bt2c::CStringView str) const;

    /*
     * Returns the bound of which the RFC 3339 date-time `str` is the
     * string representation, `match` being the match of `str` (see
     * _boundFromStr()).
     *
     * Throws `bt2c::Error` if `str` is invalid.
     */
    Bound _boundFromRfc3339Match(const bt2c::RegexMatch& match, bt2c::CStringView str) const;

    /*
     * Returns the time (nanoseconds from origin) of the date `date` at
     * the time of day `timeOfDay` in the time zone `_mTz`, or
     * `std::nullopt` if it can't convert it, including if the resulting
     * time is before the Unix epoch or if its number of nanoseconds
     * from origin doesn't fit `std::int64_t`.
     */
    std::optional<std::int64_t> _nsFromOrig(const bt2c::Date& date,
                                            const TimeOfDayWithNs& timeOfDay) const noexcept;

    /*
     * Returns the full time bound of the time of day `timeOfDay` on the
     * date, in the time zone `_mTz`, of the time `refNsFromOrig`
     * (nanoseconds from origin).
     *
     * Throws `bt2c::Error` on error.
     */
    Bound _fullBound(const TimeOfDayWithNs& timeOfDay, std::int64_t refNsFromOrig) const;

    /*
     * Validates the trimming time range having the beginning bound
     * `begin` and the end bound `end`, both not being times of day.
     *
     * Throws `bt2c::Error` if the trimming time range is invalid.
     */
    void _validateBounds(const Bound& begin, const Bound& end) const;

    static constexpr auto _inPortName = "in";

    /* Bounds of the trimming time range */
    Bound _mBegin;
    Bound _mEnd;

    /* Expected format of the `begin` and `end` parameters */
    TsFmtType _mTsFmtType = TsFmtType::Auto;

    /* Time zone of the date/time of `_mBegin` and `_mEnd` */
    bt2c::Tz _mTz = bt2c::Tz::createLocal();
};

} /* namespace bt2trimmer */

#endif /* BABELTRACE_PLUGINS_UTILS_TRIMMER_COMP_HPP */
