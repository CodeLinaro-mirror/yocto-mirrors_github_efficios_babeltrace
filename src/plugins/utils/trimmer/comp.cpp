/*
 * SPDX-License-Identifier: MIT
 *
 * Copyright 2016 Jérémie Galarneau <jeremie.galarneau@efficios.com>
 * Copyright 2019-2026 Philippe Proulx <pproulx@efficios.com>
 */

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <fmt/core.h>
#include <glib.h>

#include <babeltrace2/babeltrace.h>

#include "common/assert.h"
#include "common/common.h"
#include "common/safe.h"
#include "cpp-common/bt2/wrap.hpp"
#include "cpp-common/bt2c/c-string-view.hpp"
#include "cpp-common/bt2c/date-time.hpp"
#include "cpp-common/bt2c/fmt.hpp" /* IWYU pragma: keep */
#include "cpp-common/bt2c/glib-up.hpp"
#include "cpp-common/bt2c/regex.hpp"
#include "cpp-common/bt2c/safe-ops.hpp"
#include "cpp-common/bt2c/tz.hpp"

#include "plugins/common/param-validation/param-validation.h"

#include "comp.hpp"

namespace bt2trimmer {
namespace {

bt_param_validation_status validateBoundType(const bt_value * const libValue,
                                             bt_param_validation_context * const context)
{
    const auto val = bt2::wrap(libValue);

    if (!val.isSignedInteger() && !val.isString()) {
        return bt_param_validation_error(
            context, "unexpected type: expected-types=[%s, %s], actual-type=%s",
            bt_common_value_type_string(BT_VALUE_TYPE_SIGNED_INTEGER),
            bt_common_value_type_string(BT_VALUE_TYPE_STRING),
            bt_common_value_type_string(static_cast<bt_value_type>(val.type())));
    }

    return BT_PARAM_VALIDATION_STATUS_OK;
}

bt_param_validation_map_value_entry_descr tsFmtParams[] = {
    {"tz", BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_OPTIONAL,
     bt_param_validation_value_descr::makeString()},
    BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_END};

bt_param_validation_map_value_entry_descr trimmerParams[] = {
    {"gmt", BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_OPTIONAL,
     bt_param_validation_value_descr::makeBool()},
    {"ts-format", BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_OPTIONAL,
     bt_param_validation_value_descr::makeMap(tsFmtParams)},
    {"begin", BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_OPTIONAL,
     bt_param_validation_value_descr::makeCustom(validateBoundType)},
    {"end", BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_OPTIONAL,
     bt_param_validation_value_descr::makeCustom(validateBoundType)},
    BT_PARAM_VALIDATION_MAP_VALUE_ENTRY_END};

/*
 * Returns the value of the decimal digits `str` (not empty), or
 * `std::nullopt` if `str` contains anything else or if the value
 * doesn't fit `std::uint64_t`.
 */
std::optional<std::uint64_t> digitsVal(const std::string_view str) noexcept
{
    std::uint64_t val;
    const auto end = str.data() + str.size();
    const auto res = std::from_chars(str.data(), end, val);

    if (res.ec != std::errc {} || res.ptr != end) {
        return std::nullopt;
    }

    return val;
}

/*
 * Returns the value of the decimal digits which the group at the index
 * `index` of `match` matches, or zero if this group doesn't participate
 * in the match.
 *
 * Such a group matches one to nine decimal digits.
 */
unsigned int groupVal(const bt2c::RegexMatch& match, const std::size_t index) noexcept
{
    const auto str = match[index];

    if (!str) {
        return 0;
    }

    const auto val = digitsVal(*str);

    BT_ASSERT(val);
    return static_cast<unsigned int>(*val);
}

/*
 * Returns the number of nanoseconds of which the group at the index
 * `index` of `match` is the representation of the fractional part of a
 * second, or zero if this group doesn't participate in the match.
 *
 * For example, `512` means 512,000,000 ns, not 512 ns.
 */
unsigned int groupFracNs(const bt2c::RegexMatch& match, const std::size_t index) noexcept
{
    auto ns = groupVal(match, index);

    if (const auto str = match[index]) {
        /* Multiply by 10 as many times as there are omitted digits */
        for (auto i = str->size(); i < 9; ++i) {
            ns *= 10;
        }
    }

    return ns;
}

/*
 * Returns the date which the `YYYY`, `MM`, and `DD` groups of `match`,
 * starting at the index `firstIndex`, represent, or `std::nullopt` if
 * it's invalid.
 */
std::optional<bt2c::Date> dateFromMatch(const bt2c::RegexMatch& match,
                                        const std::size_t firstIndex) noexcept
{
    const auto year = static_cast<int>(groupVal(match, firstIndex));
    const auto month = groupVal(match, firstIndex + 1);
    const auto day = groupVal(match, firstIndex + 2);

    if (!bt2c::Date::isValid(year, month, day)) {
        return std::nullopt;
    }

    return bt2c::Date {year, month, day};
}

/*
 * Returns the time of day which the `HH`, `II`, `SS`, and `NANO` groups
 * of `match`, starting at the index `firstIndex`, represent, or
 * `std::nullopt` if it's invalid.
 */
std::optional<TimeOfDayWithNs> timeOfDayFromMatch(const bt2c::RegexMatch& match,
                                                  const std::size_t firstIndex) noexcept
{
    const auto hour = groupVal(match, firstIndex);
    const auto minute = groupVal(match, firstIndex + 1);
    const auto second = groupVal(match, firstIndex + 2);

    if (!bt2c::TimeOfDay::isValid(hour, minute, second)) {
        return std::nullopt;
    }

    return TimeOfDayWithNs {bt2c::TimeOfDay {hour, minute, second},
                            groupFracNs(match, firstIndex + 3)};
}

/*
 * Returns the time (nanoseconds from origin, the Unix epoch) of the
 * date `date` at the time of day `timeOfDay` in the time zone `tz`, or
 * `std::nullopt` if it can't convert it or if the result doesn't fit
 * `std::int64_t`.
 */
std::optional<std::int64_t> nsFromOrigInTz(const bt2c::Tz& tz, const bt2c::Date& date,
                                           const TimeOfDayWithNs& timeOfDay) noexcept
{
    const auto sec = tz.secFromDateTime(bt2c::DateTime {date, timeOfDay.timeOfDay});

    if (!sec) {
        return std::nullopt;
    }

    const auto sec64 = static_cast<std::int64_t>(*sec);
    const auto ns = static_cast<std::int64_t>(timeOfDay.ns);

    if (sec64 >= 0) {
        if (!bt_safe_to_mul_int64(sec64, NS_PER_S_I)) {
            return std::nullopt;
        }

        return bt2c::safeAdd(sec64 * NS_PER_S_I, ns);
    }

    /*
     * `sec64` × 1,000,000,000 may not fit `std::int64_t` while the
     * result does (for example, 1677-09-21T00:12:43.145224192Z):
     * compute (`sec64` + 1) × 1,000,000,000 − (1,000,000,000 − `ns`)
     * instead.
     */
    if (!bt_safe_to_mul_int64(sec64 + 1, NS_PER_S_I)) {
        return std::nullopt;
    }

    return bt2c::safeSub((sec64 + 1) * NS_PER_S_I, NS_PER_S_I - ns);
}

} /* namespace */

Comp::Comp(const bt2::SelfFilterComponent selfComp, const bt2::ConstMapValue params, void *)
    : bt2::UserFilterComponent<Comp, MsgIter> {selfComp, "PLUGIN/FLT.UTILS.TRIMMER"}
{
    this->_addInputPort(_inPortName);
    this->_addOutputPort("out");

    /* Validate parameters */
    {
        gchar *validateError = nullptr;
        const auto validationStatus =
            bt_param_validation_validate(params.libObjPtr(), trimmerParams, &validateError);
        const bt2c::GCharUP validateErrorUP {validateError};

        if (validationStatus == BT_PARAM_VALIDATION_STATUS_MEMORY_ERROR) {
            throw bt2::MemoryError {};
        } else if (validationStatus == BT_PARAM_VALIDATION_STATUS_VALIDATION_ERROR) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "{}",
                                              validateError ? validateError : "");
        }
    }

    if (const auto tsFmtVal = params["ts-format"]) {
        if (params.hasEntry("gmt")) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error, "`ts-format` and `gmt` parameters are mutually exclusive.");
        }

        if (const auto tzVal = tsFmtVal->asMap()["tz"]) {
            try {
                _mTz = bt2c::Tz::createFromStr(tzVal->asString().value(), _mLogger);
            } catch (const bt2c::Error&) {
                BT_CPPLOGE_APPEND_CAUSE_AND_RETHROW("Invalid `tz` entry of `ts-format` parameter.");
            }
        }
    } else if (const auto gmtVal = params["gmt"]; gmtVal && gmtVal->asBool().value()) {
        _mTz = bt2c::Tz::createUtc();
    }

    if (const auto beginVal = params["begin"]) {
        _mBegin = this->_boundFromParam(*beginVal);
    }

    if (const auto endVal = params["end"]) {
        _mEnd = this->_boundFromParam(*endVal);
    }

    if (!_mBegin.needsDate() && !_mEnd.needsDate()) {
        this->_validateBounds(_mBegin, _mEnd);
    }
}

void Comp::_getSupportedMipVersions(bt2::SelfComponentClass, bt2::ConstValue, void *,
                                    bt2::LoggingLevel, const bt2::UnsignedIntegerRangeSet ranges)
{
    ranges.addRange(0, 1);
}

Bound Comp::_boundFromParam(const bt2::ConstValue param) const
{
    if (param.isSignedInteger()) {
        /* Convert it to a string to handle everything the same way */
        return this->_boundFromStr(fmt::to_string(param.asSignedInteger().value()));
    }

    return this->_boundFromStr(param.asString().value());
}

Bound Comp::_boundFromStr(const bt2c::CStringView str) const
{
    /* RFC 3339 date-time: `YYYY-MM-DDTHH:II:SS[.NANO]OFFSET` */
    static const bt2c::Regex rfc3339Regex {
        /* `YYYY-MM-DD` */
        "^([0-9]{4})-([0-9]{2})-([0-9]{2})"

        /* Separator: `T` also being `t` or a space (RFC 3339, 5.6) */
        "[Tt ]"

        /* `HH:II:SS[.NANO]` */
        "([0-9]{2}):([0-9]{2}):([0-9]{2})(?:\\.([0-9]{1,9}))?"

        /* `Z`, `z`, `+HH:II`, or `-HH:II` */
        "(?:[Zz]|([+-])([0-9]{2}):([0-9]{2}))"

        "\\z"};

    /* `YYYY-MM-DD[ HH:II[:SS[.NANO]]]` */
    static const bt2c::Regex dtRegex {
        /* `YYYY-MM-DD` */
        "^([0-9]{4})-([0-9]{2})-([0-9]{2})"

        /* Optional ` HH:II[:SS[.NANO]]` */
        "(?: ([0-9]{2}):([0-9]{2})(?::([0-9]{2})(?:\\.([0-9]{1,9}))?)?)?"

        "\\z"};

    /* `HH:II[:SS[.NANO]]` */
    static const bt2c::Regex timeOfDayRegex {
        "^([0-9]{2}):([0-9]{2})(?::([0-9]{2})(?:\\.([0-9]{1,9}))?)?\\z"};

    /* `[-]SEC[.NANO]` */
    static const bt2c::Regex secRegex {"^(-?)([0-9]+)(?:\\.([0-9]{1,9}))?\\z"};

    if (const auto match = rfc3339Regex.matchGroups(str)) {
        return this->_boundFromRfc3339Match(*match, str);
    }

    if (const auto match = dtRegex.matchGroups(str)) {
        const auto date = dateFromMatch(*match, 1);

        if (!date) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid date: param=\"{}\"", str);
        }

        /* Midnight without a time of day */
        const auto timeOfDay =
            (*match)[4] ? timeOfDayFromMatch(*match, 4) : std::make_optional(TimeOfDayWithNs {});

        if (!timeOfDay) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid time of day: param=\"{}\"",
                                              str);
        }

        if (const auto nsFromOrig = this->_nsFromOrig(*date, *timeOfDay)) {
            return Bound {*nsFromOrig};
        }

        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
            bt2c::Error,
            "Cannot convert date/time to nanoseconds from origin "
            "(date/time before the Unix epoch or after 2262?): param=\"{}\"",
            str);
    }

    if (const auto match = timeOfDayRegex.matchGroups(str)) {
        if (const auto timeOfDay = timeOfDayFromMatch(*match, 1)) {
            return Bound {*timeOfDay};
        }

        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid time of day: param=\"{}\"", str);
    }

    if (const auto match = secRegex.matchGroups(str)) {
        /* The number of seconds itself may not even fit `std::uint64_t` */
        const auto sec = digitsVal(*(*match)[2]);
        const auto secNs = sec ? bt2c::safeMul<std::uint64_t>(*sec, NS_PER_S_U) : std::nullopt;
        const auto absNsFromOrig =
            secNs ? bt2c::safeAdd<std::uint64_t>(*secNs, groupFracNs(*match, 3)) : std::nullopt;

        /*
         * The absolute value of the number of nanoseconds from origin
         * must fit `std::int64_t`.
         */
        if (!absNsFromOrig ||
            *absNsFromOrig > static_cast<std::uint64_t>(std::numeric_limits<std::int64_t>::max())) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
                bt2c::Error, "Time is out of range (nanoseconds from origin): param=\"{}\"", str);
        }

        const auto absNsFromOrigI = static_cast<std::int64_t>(*absNsFromOrig);

        return Bound {(*match)[1]->empty() ? absNsFromOrigI : -absNsFromOrigI};
    }

    BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid date/time format: param=\"{}\"", str);
}

Bound Comp::_boundFromRfc3339Match(const bt2c::RegexMatch& match, const bt2c::CStringView str) const
{
    const auto date = dateFromMatch(match, 1);

    if (!date) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid date: param=\"{}\"", str);
    }

    const auto timeOfDay = timeOfDayFromMatch(match, 4);

    if (!timeOfDay) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid time of day: param=\"{}\"", str);
    }

    /*
     * UTC offset (seconds), positive east of UTC.
     *
     * `Z` means UTC, and so does `-00:00` which, as per RFC 3339, 4.3,
     * means the date/time is in UTC without knowing the local offset.
     */
    auto offsetSec = 0;

    if (const auto sign = match[8]) {
        const auto hours = groupVal(match, 9);
        const auto mins = groupVal(match, 10);

        if (hours > 23 || mins > 59) {
            BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error, "Invalid UTC offset: param=\"{}\"", str);
        }

        offsetSec = static_cast<int>(hours * 3600 + mins * 60);

        if ((*sign)[0] == '-') {
            offsetSec = -offsetSec;
        }
    }

    if (const auto nsFromOrig =
            nsFromOrigInTz(bt2c::Tz::createFixedOffset(offsetSec), *date, *timeOfDay)) {
        return Bound {*nsFromOrig};
    }

    BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
        bt2c::Error, "Date/time is out of range (nanoseconds from origin): param=\"{}\"", str);
}

std::optional<std::int64_t> Comp::_nsFromOrig(const bt2c::Date& date,
                                              const TimeOfDayWithNs& timeOfDay) const noexcept
{
    const auto nsFromOrig = nsFromOrigInTz(_mTz, date, timeOfDay);

    /* Reject a time before the Unix epoch */
    if (nsFromOrig && *nsFromOrig < 0) {
        return std::nullopt;
    }

    return nsFromOrig;
}

Bound Comp::_fullBound(const TimeOfDayWithNs& timeOfDay, const std::int64_t refNsFromOrig) const
{
    const auto refSec = refNsFromOrig / NS_PER_S_I;

    if (refSec < std::numeric_limits<std::time_t>::min() ||
        refSec > std::numeric_limits<std::time_t>::max()) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error,
                                          "Cannot convert timestamp to date and time "
                                          "(number of seconds doesn't fit `time_t`): ts={}",
                                          refSec);
    }

    const auto timeSec = static_cast<std::time_t>(refSec);

    /* We only need the date of this time */
    const auto refDt = _mTz.dateTime(timeSec);

    if (!refDt) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error,
                                          "Cannot convert timestamp to date and time: ts={}",
                                          static_cast<std::int64_t>(timeSec));
    }

    const auto& date = refDt->date();

    if (const auto nsFromOrig = this->_nsFromOrig(date, timeOfDay)) {
        return Bound {*nsFromOrig};
    }

    BT_CPPLOGE_APPEND_CAUSE_AND_THROW(bt2c::Error,
                                      "Cannot convert date/time to nanoseconds from origin "
                                      "(date/time before the Unix epoch or after 2262?): "
                                      "date-time={}.{:09}",
                                      bt2c::DateTime {date, timeOfDay.timeOfDay}, timeOfDay.ns);
}

void Comp::_validateBounds(const Bound& begin, const Bound& end) const
{
    BT_ASSERT(!begin.needsDate());
    BT_ASSERT(!end.needsDate());

    if (!begin.isInfinite() && !end.isInfinite() && begin.nsFromOrig() > end.nsFromOrig()) {
        BT_CPPLOGE_APPEND_CAUSE_AND_THROW(
            bt2c::Error,
            "Trimming time range's beginning time is greater than end time: "
            "begin-ns-from-origin={}, end-ns-from-origin={}",
            begin.nsFromOrig(), end.nsFromOrig());
    }
}

} /* namespace bt2trimmer */
