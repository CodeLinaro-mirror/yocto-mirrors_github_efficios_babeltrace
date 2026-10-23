/*
 * Copyright (c) 2026 EfficiOS, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "date-time.hpp"

namespace bt2c {
namespace {

/*
 * Returns whether or not `year` is a leap year of the proleptic
 * Gregorian calendar.
 */
bool isLeapYear(const int year) noexcept
{
    return year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
}

} /* namespace */

bool Date::isValid(const int year, const unsigned int month, const unsigned int day) noexcept
{
    static constexpr unsigned int daysPerMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

    if (month < 1 || month > 12 || day < 1) {
        return false;
    }

    return day <= daysPerMonth[month - 1] + (month == 2 && isLeapYear(year) ? 1 : 0);
}

} /* namespace bt2c */
